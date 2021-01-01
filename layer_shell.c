#include "layer_shell.h"

#include <stdlib.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/util/log.h>

#include "xdg_shell.h"

#include "arrange.h"
#include "output.h"
#include "server.h"

void
handle_layer_shell_surface_commit(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell *layer_shell = wl_container_of(listener, layer_shell, commit);
	struct wlr_layer_surface_v1 *wlr_layer_surface = layer_shell->wlr_layer_surface;
	struct cg_output *output = (struct cg_output *) wlr_layer_surface->output->data;

	wlr_log(WLR_DEBUG, "Layer surface commited (%s)", wlr_layer_surface->namespace);

	output_damage_surface(output, wlr_layer_surface->surface, layer_shell->geo.x, layer_shell->geo.y, true);
}

static void
handle_layer_shell_surface_map(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell *layer_shell = wl_container_of(listener, layer_shell, map);
	wlr_log(WLR_DEBUG, "Layer surface mapped (%s)", layer_shell->wlr_layer_surface->namespace);

	// layer_shell->commit.notify = handle_layer_shell_surface_commit;
	// wl_signal_add(&layer_shell->wlr_layer_surface->surface->events.commit, &layer_shell->commit);

	// view_map(view, layer_shell->wlr_layer_surface->surface);

	// view_damage_whole(view);
}

static void
handle_layer_shell_surface_unmap(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell *layer_shell = wl_container_of(listener, layer_shell, unmap);
	// struct cg_view *view = &layer_shell->view;
	wlr_log(WLR_DEBUG, "Layer surface unmapped (%s)", layer_shell->wlr_layer_surface->namespace);

	// view_damage_whole(view);

	// wl_list_remove(&layer_shell->commit.link);

	// view_unmap(view);
}

static void
handle_layer_shell_surface_destroy(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell *layer_shell = wl_container_of(listener, layer_shell, destroy);
	wlr_log(WLR_DEBUG, "Layer surface destroyed (%s)", layer_shell->wlr_layer_surface->namespace);

	wl_list_remove(&layer_shell->map.link);
	wl_list_remove(&layer_shell->unmap.link);
	wl_list_remove(&layer_shell->destroy.link);
	wl_list_remove(&layer_shell->link);

	layer_shell->wlr_layer_surface = NULL;

	// view_destroy(view);
}

void
handle_layer_shell_surface_new(struct wl_listener *listener, void *data)
{
	struct wlr_layer_surface_v1 *wlr_layer_surface = data;
	wlr_log(WLR_DEBUG,
		"new layer surface: namespace %s layer %d anchor %" PRIu32 " size %" PRIu32 "x%" PRIu32
		" margin %" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",",
		wlr_layer_surface->namespace, wlr_layer_surface->client_pending.layer,
		wlr_layer_surface->client_pending.anchor, wlr_layer_surface->client_pending.desired_width,
		wlr_layer_surface->client_pending.desired_height, wlr_layer_surface->client_pending.margin.top,
		wlr_layer_surface->client_pending.margin.right, wlr_layer_surface->client_pending.margin.bottom,
		wlr_layer_surface->client_pending.margin.left);

	struct cg_server *server = wl_container_of(listener, server, new_layer_shell_surface);
	if (!wlr_layer_surface->output) {
		wlr_log(WLR_DEBUG, "no output assigned to layer surface. assigning something");
		struct wlr_output_layout *output_layout = server->output_layout;
		// TODO
		// we just use first output we find for now
		wlr_layer_surface->output = wlr_output_layout_output_at(output_layout, 0, 0);
	}

	struct cg_layer_shell *layer_shell = calloc(1, sizeof(struct cg_layer_shell));

	layer_shell->wlr_layer_surface = wlr_layer_surface;

	// layer_surface->seat = server->seat;
	layer_shell->map.notify = handle_layer_shell_surface_map;
	wl_signal_add(&wlr_layer_surface->events.map, &layer_shell->map);
	layer_shell->unmap.notify = handle_layer_shell_surface_unmap;
	wl_signal_add(&wlr_layer_surface->events.unmap, &layer_shell->unmap);
	layer_shell->commit.notify = handle_layer_shell_surface_commit;
	wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer_shell->commit);
	layer_shell->destroy.notify = handle_layer_shell_surface_destroy;
	wl_signal_add(&wlr_layer_surface->events.destroy, &layer_shell->destroy);

	// // find the related cg_output....sway and dwl have their output struct in wlr_output->data
	// // in cage we need to iterate all outputs

	struct cg_output *output = wlr_layer_surface->output->data;
	wl_list_insert(&output->layers[wlr_layer_surface->client_pending.layer], &layer_shell->link);

	// // Temporarily set the layer's current state to client_pending
	// // so that we can easily arrange it
	struct wlr_layer_surface_v1_state old_state = wlr_layer_surface->current;
	arrange_server(server);
	wlr_layer_surface->current = wlr_layer_surface->client_pending;
	wlr_layer_surface->current = old_state;
}
