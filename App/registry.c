//
// Created by Cherry on 2026/6/11.
//

#include "registry.h"
#include <string.h>

static const registry_t REGISTRY_DEFAULTS = {
    .usb_mode = USB_MODE_BASIC,   /* 需 #include "usb_mode.h" */
    .click_enabled = 0,
};

static registry_t cache;

//
// Load the persisted settings into the RAM cache:
//   1. initialize the cache to defaults,
//   2. storage_load the registry object on top (length-overlay; see rules),
//   3. if absent or unreadable, the defaults remain.
//
// Must run AFTER storage_mount and BEFORE any reader (e.g. usb_mode_init).
//
void registry_init(void)
{
    uint16_t instance = 0;
    cache = REGISTRY_DEFAULTS;

    storage_load(STORAGE_TYPE_REGISTRY, instance, &cache, sizeof(registry_t));
}

//
// Access the RAM cache. Reads are cheap (no flash access); callers may read
// fields directly. A bare field write only changes RAM — call registry_save()
// afterwards to persist.
//
registry_t *registry(void)
{
    return &cache;
}

//
// Write the current RAM cache back to the storage layer (write-through).
//
void registry_save(void)
{
    storage_save(STORAGE_TYPE_REGISTRY, 0, &cache, sizeof(registry_t));
}