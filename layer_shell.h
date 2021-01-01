#ifndef CG_LAYER_SHELL_H
#define CG_LAYER_SHELL_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "seat.h"

struct cg_layer_shell {
	struct wlr_layer_surface_v1 *wlr_layer_surface;

	// struct cg_seat *seat;
	struct wl_list link;

	struct wl_listener destroy;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;

	struct wlr_box geo;
	enum zwlr_layer_shell_v1_layer layer;
};

void handle_layer_shell_surface_new(struct wl_listener *listener, void *data);

#endif
