//
// App/registry.h
//
// Device settings registry: a single RAM-cached struct of persistent
// settings, backed by one object in the storage layer. This sits ABOVE the
// storage layer — storage handles opaque bytes, wear-leveling and crash
// safety; the registry knows WHAT the settings are and their defaults.
//

#ifndef REGISTRY_H
#define REGISTRY_H

#include <stdint.h>
#include "storage_types.h"
#include "storage.h"
#include "usb_mode.h"

//
// Persistent settings.
//
// SCHEMA RULES — read before changing this struct:
//
//   * APPEND ONLY. Add new settings at the BOTTOM. Never reorder, resize,
//     or change the meaning of an existing field. The load path overlays a
//     stored (possibly older / shorter) blob onto a defaults-initialized
//     struct BY BYTE OFFSET, so every existing field must keep its offset
//     and meaning forever.
//
//   * Adding a field is backward-compatible for free: an older, shorter
//     stored blob loads into the leading fields; any newer field keeps its
//     default.
//
//   * For an INCOMPATIBLE change (resize / repurpose / remove a field),
//     do NOT version this struct in place. Instead change the storage key
//     used in registry.c (e.g. bump the instance id) so old objects are
//     simply not found and the whole struct falls back to defaults.
//
typedef struct {
    uint8_t usb_mode;   // usb_mode_t; default = USB_MODE_BASIC
    // ── append new settings below this line only ──
} registry_t;

//
// Load the persisted settings into the RAM cache:
//   1. initialize the cache to defaults,
//   2. storage_load the registry object on top (length-overlay; see rules),
//   3. if absent or unreadable, the defaults remain.
//
// Must run AFTER storage_mount and BEFORE any reader (e.g. usb_mode_init).
//
void registry_init(void);

//
// Access the RAM cache. Reads are cheap (no flash access); callers may read
// fields directly. A bare field write only changes RAM — call registry_save()
// afterwards to persist.
//
registry_t *registry(void);

//
// Write the current RAM cache back to the storage layer (write-through).
//
void registry_save(void);

#endif /* REGISTRY_H */