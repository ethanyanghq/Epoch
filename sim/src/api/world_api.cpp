#include "alpha/api/world_api.hpp"

namespace alpha {

CreateWorldResult WorldApi::create_world(const WorldCreateParams& params) {
  return simulation_.create_world(params);
}

TurnReport WorldApi::advance_month() {
  return simulation_.advance_month();
}

ChunkVisualResult WorldApi::get_chunk_visual(const ChunkVisualQuery& query) const {
  return simulation_.get_chunk_visual(query);
}

WorldMetrics WorldApi::get_world_metrics() const {
  return simulation_.get_world_metrics();
}

}  // namespace alpha
