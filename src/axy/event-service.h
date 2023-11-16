#ifndef AXY_SRC_AXY_EVENT_SERVICE_H_
#define AXY_SRC_AXY_EVENT_SERVICE_H_

#include <sdifi/events/v1alpha/event.grpc.pb.h>
#include <sw/redis++/redis.h>

#include <memory>
#include <thread>

#include "src/axy/logging.h"

namespace axy {

class EventServiceImpl final
    : public sdifi::events::v1alpha::EventService::CallbackService {
 public:
  explicit EventServiceImpl(std::shared_ptr<sw::redis::Redis> redis);

  ~EventServiceImpl() = default;

  grpc::ServerWriteReactor<sdifi::events::v1alpha::WatchResponse>* Watch(
      grpc::CallbackServerContext* context,
      const sdifi::events::v1alpha::WatchRequest* request) override;

 private:
  // TODO(rkjaran): migrate to AsyncRedis once we can reliably link to
  //   hired>1.0.0
  std::shared_ptr<sw::redis::Redis> redis_;
};

}  // namespace axy

#endif  // AXY_SRC_AXY_EVENT_SERVICE_H_
