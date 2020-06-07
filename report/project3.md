# Buffer Cache
Changes to design are as follows:
- Add implementation for cached block read and write without offset and size (complete sector), implementation with offset is ended with partial suffix.
- Add 3 syscalls (`fs_info`, `cache_info`, `cache_flush`) for user tests and also track stats about fs read/write and cache access/hit for them.
- Add some functions to make code more clean. Most importantly `fetch_cache_block` which fetches cache block if it exists in the cache and `create_cache_block` which creates a cache block (without reading it) and adds it to the LRU block list and removes least recently used block if size of cache exceeds the limit.
