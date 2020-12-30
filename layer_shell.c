#include "layer_shell.h"

#include <stdlib.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/util/log.h>

#include "output.h"
#include "server.h"

static void
apply_exclusive(struct wlr_box *usable_area, uint32_t anchor, int32_t exclusive, int32_t margin_top,
		int32_t margin_right, int32_t margin_bottom, int32_t margin_left)
{
	if (exclusive <= 0) {
		return;
	}
	struct {
		uint32_t singular_anchor;
		uint32_t anchor_triplet;
		int *positive_axis;
		int *negative_axis;
		int margin;
	} edges[] = {
		// Top
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
			.positive_axis = &usable_area->y,
			.negative_axis = &usable_area->height,
			.margin = margin_top,
		},
		// Bottom
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.positive_axis = NULL,
			.negative_axis = &usable_area->height,
			.margin = margin_bottom,
		},
		// Left
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.positive_axis = &usable_area->x,
			.negative_axis = &usable_area->width,
			.margin = margin_left,
		},
		// Right
		{
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
			.anchor_triplet = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
					  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.positive_axis = NULL,
			.negative_axis = &usable_area->width,
			.margin = margin_right,
		},
	};
	for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
		if ((anchor == edges[i].singular_anchor || anchor == edges[i].anchor_triplet) &&
		    exclusive + edges[i].margin > 0) {
			if (edges[i].positive_axis) {
				*edges[i].positive_axis += exclusive + edges[i].margin;
			}
			if (edges[i].negative_axis) {
				*edges[i].negative_axis -= exclusive + edges[i].margin;
			}
			break;
		}
	}
}

void
arrange_output(struct cg_output *output)
{
	// const struct wlr_box *output_box = wlr_output_layout_get_box(root->output_layout, output->wlr_output);
	// output->lx = 20.0;
	// output->ly = 70.0;
	// output->width = output_box->width;
	// output->height = output_box->height;

	// for (int i = 0; i < output->workspaces->length; ++i) {
	// 	struct sway_workspace *workspace = output->workspaces->items[i];
	// 	arrange_workspace(workspace);
	// }
}

static void
arrange_layer(struct cg_output *output, struct wl_list *list, struct wlr_box *usable_area, bool exclusive)
{
	// struct cg_layer_surface *cg_layer;
	// struct wlr_box full_area = {0};
	// wlr_output_effective_resolution(output->wlr_output, &full_area.width, &full_area.height);
	// wl_list_for_each (cg_layer, list, link) {
	// 	struct wlr_layer_surface_v1 *layer = cg_layer->wlr_layer_surface;
	// 	struct wlr_layer_surface_v1_state *state = &layer->current;
	// 	if (exclusive != (state->exclusive_zone > 0)) {
	// 		continue;
	// 	}
	// 	struct wlr_box bounds;
	// 	if (state->exclusive_zone == -1) {
	// 		bounds = full_area;
	// 	} else {
	// 		bounds = *usable_area;
	// 	}
	// 	struct wlr_box box = {.width = state->desired_width, .height = state->desired_height};
	// 	// Horizontal axis
	// 	const uint32_t both_horiz = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
	// 	if ((state->anchor & both_horiz) && box.width == 0) {
	// 		box.x = bounds.x;
	// 		box.width = bounds.width;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
	// 		box.x = bounds.x;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)) {
	// 		box.x = bounds.x + (bounds.width - box.width);
	// 	} else {
	// 		box.x = bounds.x + ((bounds.width / 2) - (box.width / 2));
	// 	}
	// 	// Vertical axis
	// 	const uint32_t both_vert = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
	// 	if ((state->anchor & both_vert) && box.height == 0) {
	// 		box.y = bounds.y;
	// 		box.height = bounds.height;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
	// 		box.y = bounds.y;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)) {
	// 		box.y = bounds.y + (bounds.height - box.height);
	// 	} else {
	// 		box.y = bounds.y + ((bounds.height / 2) - (box.height / 2));
	// 	}
	// 	// Margin
	// 	if ((state->anchor & both_horiz) == both_horiz) {
	// 		box.x += state->margin.left;
	// 		box.width -= state->margin.left + state->margin.right;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
	// 		box.x += state->margin.left;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)) {
	// 		box.x -= state->margin.right;
	// 	}
	// 	if ((state->anchor & both_vert) == both_vert) {
	// 		box.y += state->margin.top;
	// 		box.height -= state->margin.top + state->margin.bottom;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
	// 		box.y += state->margin.top;
	// 	} else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)) {
	// 		box.y -= state->margin.bottom;
	// 	}
	// 	if (box.width < 0 || box.height < 0) {
	// 		// TODO: Bubble up a protocol error?
	// 		wlr_layer_surface_v1_close(layer);
	// 		continue;
	// 	}
	// 	// Apply
	// 	cg_layer->geo = box;
	// 	apply_exclusive(usable_area, state->anchor, state->exclusive_zone, state->margin.top,
	// 			state->margin.right, state->margin.bottom, state->margin.left);
	// 	wlr_layer_surface_v1_configure(layer, box.width, box.height);
	// }
}

void
arrange_layers(struct cg_output *output)
{
	return;
	struct wlr_box usable_area = {0};
	wlr_output_effective_resolution(output->wlr_output, &usable_area.width, &usable_area.height);

	// Arrange exclusive surfaces from top->bottom
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &usable_area, true);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &usable_area, true);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], &usable_area, true);
	arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], &usable_area, true);

	// if (memcmp(&usable_area, &output->usable_area, sizeof(struct wlr_box)) != 0) {
	// 	wlr_log(WLR_DEBUG, "Usable area changed, rearranging output");
	// 	memcpy(&output->usable_area, &usable_area, sizeof(struct wlr_box));
	// 	arrange_output(output);
	// }

	// // Arrange non-exlusive surfaces from top->bottom
	// arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &usable_area, false);
	// arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &usable_area, false);
	// arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], &usable_area, false);
	// arrange_layer(output, &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], &usable_area, false);

	// // Find topmost keyboard interactive layer, if such a layer exists
	// uint32_t layers_above_shell[] = {
	// 	ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
	// 	ZWLR_LAYER_SHELL_V1_LAYER_TOP,
	// };
	// size_t nlayers = sizeof(layers_above_shell) / sizeof(layers_above_shell[0]);
	// struct cg_layer_surface *layer, *topmost = NULL;
	// for (size_t i = 0; i < nlayers; ++i) {
	// 	wl_list_for_each_reverse (layer, &output->layers[layers_above_shell[i]], link) {
	// 		if (layer->wlr_layer_surface->current.keyboard_interactive &&
	// 		    layer->wlr_layer_surface->mapped) {
	// 			topmost = layer;
	// 			break;
	// 		}
	// 	}
	// 	if (topmost != NULL) {
	// 		break;
	// 	}
	// }

	// struct sway_seat *seat;
	// wl_list_for_each (seat, &server.input->seats, link) {
	// 	if (topmost != NULL) {
	// 		seat_set_focus_layer(seat, topmost->layer_surface);
	// 	} else if (seat->focused_layer && !seat->focused_layer->current.keyboard_interactive) {
	// 		seat_set_focus_layer(seat, NULL);
	// 	}
	// }
}

void
handle_layer_shell_surface_commit(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell_view *layer_shell_view = wl_container_of(listener, layer_shell_view, commit);
	struct cg_view *view = &layer_shell_view->view;

	wlr_log(WLR_DEBUG, "Layer surface commited (%s)", layer_shell_view->wlr_layer_surface->namespace);

	// view_damage_part(view);
}

static void
handle_layer_shell_surface_map(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell_view *layer_shell_view = wl_container_of(listener, layer_shell_view, map);
	struct cg_view *view = &layer_shell_view->view;
	wlr_log(WLR_DEBUG, "Layer surface mapped (%s)", layer_shell_view->wlr_layer_surface->namespace);

	// layer_shell_view->commit.notify = handle_layer_shell_surface_commit;
	// wl_signal_add(&layer_shell_view->wlr_layer_surface->surface->events.commit, &layer_shell_view->commit);

	view_map(view, layer_shell_view->wlr_layer_surface->surface);

	view_damage_whole(view);
}

static void
handle_layer_shell_surface_unmap(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell_view *layer_shell_view = wl_container_of(listener, layer_shell_view, unmap);
	struct cg_view *view = &layer_shell_view->view;
	wlr_log(WLR_DEBUG, "Layer surface unmapped (%s)", layer_shell_view->wlr_layer_surface->namespace);

	view_damage_whole(view);

	wl_list_remove(&layer_shell_view->commit.link);

	view_unmap(view);
}

static void
handle_layer_shell_surface_destroy(struct wl_listener *listener, void *data)
{
	struct cg_layer_shell_view *layer_shell_view = wl_container_of(listener, layer_shell_view, destroy);
	struct cg_view *view = &layer_shell_view->view;

	wl_list_remove(&layer_shell_view->map.link);
	wl_list_remove(&layer_shell_view->unmap.link);
	wl_list_remove(&layer_shell_view->destroy.link);
	wl_list_remove(&layer_shell_view->link);

	layer_shell_view->wlr_layer_surface = NULL;

	view_destroy(view);
}

static struct cg_layer_shell_view *
layer_shell_view_from_view(struct cg_view *view)
{
	return (struct cg_layer_shell_view *) view;
}

static char *
get_title(struct cg_view *view)
{
	return "layer shell";
}

static void
get_geometry(struct cg_view *view, int *width_out, int *height_out)
{
	struct cg_layer_shell_view *layer_shell_view = layer_shell_view_from_view(view);

	*width_out = layer_shell_view->geo.width;
	*height_out = layer_shell_view->geo.height;
}

static bool
is_primary(struct cg_view *view)
{
	// PRIMARY ALL THE THINGS (no idea :S)
	return true;
}

static bool
is_transient_for(struct cg_view *child, struct cg_view *parent)
{
	// unclear :S
	return false;
}

static void
activate(struct cg_view *view, bool activate)
{
	// unclear :S
}

static void
maximize(struct cg_view *view, int output_width, int output_height)
{
	// unclear :S
}

static void
destroy(struct cg_view *view)
{
	struct cg_layer_shell_view *layer_shell_view = layer_shell_view_from_view(view);
	free(layer_shell_view);
}

static void
for_each_surface(struct cg_view *view, wlr_surface_iterator_func_t iterator, void *data)
{
	struct cg_layer_shell_view *layer_shell_view = layer_shell_view_from_view(view);
	wlr_layer_surface_v1_for_each_surface(layer_shell_view->wlr_layer_surface, iterator, data);
}

static void
for_each_popup(struct cg_view *view, wlr_surface_iterator_func_t iterator, void *data)
{
	struct cg_layer_shell_view *layer_shell_view = layer_shell_view_from_view(view);
	wlr_layer_surface_v1_for_each_popup(layer_shell_view->wlr_layer_surface, iterator, data);
}

static struct wlr_surface *
wlr_surface_at(struct cg_view *view, double sx, double sy, double *sub_x, double *sub_y)
{
	struct cg_layer_shell_view *layer_shell_view = layer_shell_view_from_view(view);
	return wlr_layer_surface_v1_surface_at(layer_shell_view->wlr_layer_surface, sx, sy, sub_x, sub_y);
}

static const struct cg_view_impl layer_shell_view_impl = {
	.get_title = get_title,
	.get_geometry = get_geometry,
	.is_primary = is_primary,
	.is_transient_for = is_transient_for,
	.activate = activate,
	.maximize = maximize,
	.destroy = destroy,
	.for_each_surface = for_each_surface,
	.for_each_popup = for_each_popup,
	.wlr_surface_at = wlr_surface_at,
};

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

	struct cg_layer_shell_view *layer_shell_view = calloc(1, sizeof(struct cg_layer_shell_view));

	view_init(&layer_shell_view->view, server, CAGE_LAYER_SHELL_VIEW, &layer_shell_view_impl);
	layer_shell_view->wlr_layer_surface = wlr_layer_surface;

	// layer_surface->seat = server->seat;
	layer_shell_view->map.notify = handle_layer_shell_surface_map;
	wl_signal_add(&wlr_layer_surface->events.map, &layer_shell_view->map);
	layer_shell_view->unmap.notify = handle_layer_shell_surface_unmap;
	wl_signal_add(&wlr_layer_surface->events.unmap, &layer_shell_view->unmap);
	layer_shell_view->commit.notify = handle_layer_shell_surface_commit;
	wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer_shell_view->commit);
	layer_shell_view->destroy.notify = handle_layer_shell_surface_destroy;
	wl_signal_add(&wlr_layer_surface->events.destroy, &layer_shell_view->destroy);

	// // find the related cg_output....sway and dwl have their output struct in wlr_output->data
	// // in cage we need to iterate all outputs

	// XXX
	wlr_layer_surface_v1_configure(wlr_layer_surface, 400, 200);

	struct cg_output *output = wlr_layer_surface->output->data;
	wl_list_insert(&output->layers[wlr_layer_surface->client_pending.layer], &layer_shell_view->link);

	// Temporarily set the layer's current state to client_pending
	// so that we can easily arrange it
	struct wlr_layer_surface_v1_state old_state = wlr_layer_surface->current;
	arrange_layers(output);
	wlr_layer_surface->current = wlr_layer_surface->client_pending;
	wlr_layer_surface->current = old_state;
}
