//
// Created by Cherry on 2026/6/12.
//

#include "click.h"
#include "registry.h"
#include "sun_io.h"

static void click_apply(bool on)
{
    if (on) sun_io_click_on(); else sun_io_click_off();
}

void click_init(void)
{
    click_apply(registry()->click_enabled != 0);
}

void click_set(bool enabled)
{
    registry()->click_enabled = enabled ? 1 : 0;
    registry_save();
    click_apply(enabled);
}