#include "src/axy/event-service.h"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/support/status.h>
#include <sdifi/events/v1alpha/event.pb.h>
#include <sw/redis++/redis.h>

#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "src/axy/logging.h"

namespace axy {

namespace {

constexpr auto kContentKey = ":content";
constexpr auto kTypeKey = ":type";

}  // namespace

EventServiceImpl::EventServiceImpl(std::shared_ptr<sw::redis::Redis> redis)
    : redis_{std::move(redis)} {}

grpc::ServerWriteReactor<sdifi::events::v1alpha::WatchResponse>*
EventServiceImpl::Watch(grpc::CallbackServerContext* context,
                        const sdifi::events::v1alpha::WatchRequest* request) {
  class Writer
      : public grpc::ServerWriteReactor<sdifi::events::v1alpha::WatchResponse> {
   public:
    Writer(sw::redis::Redis& redis,
           const sdifi::events::v1alpha::WatchRequest* request)
        : redis_{redis}, request_{request} {
      StartWriterThread();
    }

    void OnWriteDone(bool ok) override {
      if (!ok) {
        SafelyFinish(grpc::Status::CANCELLED);
      } else {
        { std::unique_lock<std::mutex> l{res_mtx_}; }
        res_cv_.notify_one();
      }
      AXY_LOG_DEBUG("Watch Write done.");
    }

    void OnDone() override {
      AXY_LOG_INFO("Event Watch done for conversation '{}'.",
                   request_->conversation_id());
      delete this;
    }

    void OnCancel() override {
      AXY_LOG_INFO("Watch cancelled for conversation '{}'.",
                   request_->conversation_id());
      SafelyFinish(grpc::Status::CANCELLED);
    }

   private:
    using Attrs = std::unordered_map<std::string, std::string>;
    using Item = std::pair<std::string, std::optional<Attrs>>;
    using ItemStream = std::vector<Item>;

    void SafelyFinish(grpc::Status s) {
      std::lock_guard<std::mutex> lg{finished_mtx_};
      finished_ = true;
      Finish(std::move(s));
    }

    void StartWriterThread() {
      if (request_->conversation_id().empty()) {
        return SafelyFinish({grpc::StatusCode::INVALID_ARGUMENT,
                             "Field `conversation_id` cannot be empty"});
      }

      stream_key_ =
          fmt::format("sdifi/conversation/{}", request_->conversation_id());

      redis_executor_thread_ = std::jthread{[this](std::stop_token stop) {
        try {
          std::optional<std::string> last_id = std::nullopt;

          AXY_LOG_DEBUG("Started redis executor thread");

          std::set<std::string_view> match_filter{};
          for (const auto& event_type : request_->watch_event_type()) {
            match_filter.emplace(event_type);
          }

          while (!stop.stop_requested()) {
            while (!last_id) {
              ItemStream result;
              redis_.xrevrange(stream_key_, "+", "-", 1,
                               std::back_inserter(result));

              if (!result.empty()) {
                last_id = result.at(0).first;
              }
              std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }

            std::unordered_map<std::string, ItemStream> result;
            redis_.xread(stream_key_, *last_id, std::chrono::seconds{0},
                         std::inserter(result, result.end()));

            if (const auto it = result.find(stream_key_);
                !result.empty() && it != result.cend()) {
              const auto& stream = it->second;
              last_id = stream.back().first;

              for (const auto& [id, attrs] : stream) {
                if (!attrs) {
                  continue;
                }
                AXY_LOG_TRACE("Got attrs in stream: {}", *attrs);

                const auto type_it = attrs->find(kTypeKey);
                const auto content_it = attrs->find(kContentKey);
                if (type_it != attrs->cend() && content_it != attrs->cend()) {
                  std::string_view type = type_it->second;
                  const auto pos = type.find_last_of('.');
                  std::string_view possibly_short_type =
                      pos == std::string_view::npos ? "" : type.substr(pos + 1);

                  if (!match_filter.empty() &&
                      !(match_filter.contains(type) ||
                        match_filter.contains(possibly_short_type))) {
                    AXY_LOG_DEBUG("Not watching this event: '{}'",
                                  type_it->second);
                  } else if (!res_.mutable_event()->ParseFromString(
                                 content_it->second)) {
                    AXY_LOG_WARN(
                        "Couldn't parse serialized event in stream = '{}'. "
                        "Ignoring...",
                        stream_key_);
                  } else {
                    AXY_LOG_TRACE("Got serialized content: {}",
                                  content_it->second);

                    StartWrite(&res_);

                    std::unique_lock<std::mutex> l{res_mtx_};
                    res_cv_.wait(l);
                  }
                } else {
                  AXY_LOG_TRACE(
                      "Got message missing either '{}' or '{}' attrs. "
                      "Ignoring..",
                      kTypeKey, kContentKey);
                }
              }
            }
          }
        } catch (const std::exception& e) {
          AXY_LOG_ERROR("Unhandled exception in redis executor thread: {}",
                        e.what());
          SafelyFinish({grpc::StatusCode::INTERNAL, "baad stuff man."});
        }
      }};
    }

    std::string stream_key_;
    sw::redis::Redis& redis_;
    const sdifi::events::v1alpha::WatchRequest* request_;

    std::mutex res_mtx_;
    std::condition_variable res_cv_;
    bool res_written_ = true;
    sdifi::events::v1alpha::WatchResponse res_;

    std::jthread redis_executor_thread_;

    std::mutex finished_mtx_;
    bool finished_ = false;
  };

  return new Writer(*redis_, request);
}

}  // namespace axy
