//  Copyright 2019 U.C. Berkeley RISE Lab
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include "causal/causal_cache_utils.hpp"

void periodic_migration_handler(
    const StoreType &unmerged_store, InPreparationType &in_preparation,
    StoreType &causal_cut_store, VersionStoreType &version_store,
    map<Address, PendingClientMetadata> &pending_multi_key_metadata,
    map<Key, set<Key>> &to_fetch_map,
    map<Key, std::unordered_map<VectorClock, set<Key>, VectorClockHash>>
        &cover_map,
    SocketCache &pushers, KvsClientInterface *client,
    const CausalCacheThread &cct,
    map<string, set<Address>> &client_id_to_address_map, logger log) {
  for (const auto &pair : unmerged_store) {
    if ((causal_cut_store.find(pair.first) == causal_cut_store.end() ||
         causal_comparison(causal_cut_store[pair.first], pair.second) !=
             kCausalGreaterOrEqual) &&
        find_lattice_from_in_preparation(in_preparation, pair.first) ==
            nullptr) {
      to_fetch_map[pair.first] = set<Key>();
      in_preparation[pair.first].second[pair.first] = pair.second;
      recursive_dependencies_check(pair.first, pair.second, in_preparation,
                                   causal_cut_store, unmerged_store,
                                   to_fetch_map, cover_map, client, log);
      if (to_fetch_map[pair.first].size() == 0) {
        // all dependencies met
        merge_into_causal_cut(pair.first, causal_cut_store, in_preparation,
                              version_store, pending_multi_key_metadata,
                              pushers, cct, client_id_to_address_map, log,
                              unmerged_store);
        to_fetch_map.erase(pair.first);
      }
    }
  }
}
