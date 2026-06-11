/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * DBusMenu (com.canonical.dbusmenu) server adapter.
 *
 * Translates GMenuModel + GActionGroup into the DBusMenu wire protocol
 * so that KDE Plasma, GNOME appindicator, XFCE, MATE, Budgie, and
 * other SNI hosts can render context menus.
 *
 * ID scheme: position-encoded.
 *   Root = 0.
 *   Child at position p of parent ID pid = (pid << 10) | (p + 1).
 *   Max 1024 children per submenu, ~10 levels deep using int32 range.
 *
 * Structural changes → LayoutUpdated(0) (full tree invalidation).
 * Property-only changes → ItemsPropertiesUpdated (coalesced in idle).
 */

#include "gsni-dbusmenu.h"

#define DBUSMENU_IFACE          "com.canonical.dbusmenu"
#define DBUSMENU_MAX_CHILDREN   1023
#define DBUSMENU_VERSION        3

/*
 * Position-encoded ID helpers.
 */
static inline guint32
encode_id(guint32 parent, guint pos)
{
    if (parent == 0 && pos == 0)
        return 1;
    return (parent << 10) | ((pos + 1) & 0x3FF);
}

static inline guint32
decode_parent(guint32 id)
{
    if (id <= 1)
        return 0;
    return id >> 10;
}

static inline guint
decode_position(guint32 id)
{
    if (id == 0)
        return 0;
    return (id & 0x3FF) - 1;
}

/*
 * Embedded com.canonical.dbusmenu introspection XML.
 */
static const gchar dbusmenu_xml[] =
    "<node>"
    "  <interface name='com.canonical.dbusmenu'>"
    "    <property name='Version' type='u' access='read'/>"
    "    <property name='TextDirection' type='s' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <property name='IconThemePath' type='as' access='read'/>"
    "    <method name='GetLayout'>"
    "      <arg type='i'  name='parentId'       direction='in'/>"
    "      <arg type='i'  name='recursionDepth' direction='in'/>"
    "      <arg type='as' name='propertyNames'  direction='in'/>"
    "      <arg type='u'  name='revision'       direction='out'/>"
    "      <arg type='(ia{sv}av)' name='layout' direction='out'/>"
    "    </method>"
    "    <method name='GetGroupProperties'>"
    "      <arg type='ai'      name='ids'          direction='in'/>"
    "      <arg type='as'      name='propertyNames' direction='in'/>"
    "      <arg type='a(ia{sv})' name='properties' direction='out'/>"
    "    </method>"
    "    <method name='GetProperty'>"
    "      <arg type='i' name='id'   direction='in'/>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='v' name='value' direction='out'/>"
    "    </method>"
    "    <method name='Event'>"
    "      <arg type='i' name='id'        direction='in'/>"
    "      <arg type='s' name='eventId'   direction='in'/>"
    "      <arg type='v' name='data'      direction='in'/>"
    "      <arg type='u' name='timestamp' direction='in'/>"
    "    </method>"
    "    <method name='EventGroup'>"
    "      <arg type='a(isvu)' name='events'   direction='in'/>"
    "      <arg type='ai'      name='idErrors' direction='out'/>"
    "    </method>"
    "    <method name='AboutToShow'>"
    "      <arg type='i' name='id'         direction='in'/>"
    "      <arg type='b' name='needUpdate' direction='out'/>"
    "    </method>"
    "    <method name='AboutToShowGroup'>"
    "      <arg type='ai' name='ids'           direction='in'/>"
    "      <arg type='ai' name='updatesNeeded' direction='out'/>"
    "      <arg type='ai' name='idErrors'      direction='out'/>"
    "    </method>"
    "    <signal name='ItemsPropertiesUpdated'>"
    "      <arg type='a(ia{sv})' name='updatedProps'/>"
    "      <arg type='a(ias)'    name='removedProps'/>"
    "    </signal>"
    "    <signal name='LayoutUpdated'>"
    "      <arg type='u' name='revision'/>"
    "      <arg type='i' name='parent'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static GDBusInterfaceInfo *dbusmenu_iface_info = NULL;

struct _GsniDBusMenu {
    GDBusConnection *connection;
    gchar           *object_path;
    GMenuModel      *menu_model;
    GActionGroup    *action_group;

    guint            reg_id;
    guint32          revision;

    gulong           menu_changed_id;
    gulong           action_state_id;
    gulong           action_enabled_id;

    /* Idle coalescing */
    guint            layout_idle_id;
    guint            props_idle_id;

    /* Pending property changes for ItemsPropertiesUpdated coalescing */
    GHashTable      *pending_updates;  /* GINT_TO_POINTER(id) → GHashTable */
};

/* Forward declarations */
static GVariant *build_layout_recursive(GsniDBusMenu *self,
                                        GMenuModel *model,
                                        guint32 parent_id,
                                        gint recurrence,
                                        const gchar *const *props,
                                        guint n_props,
                                        gboolean root_section);
static void
schedule_layout_update(GsniDBusMenu *self);

/* --- D-Bus method dispatch --- */

static void
dbusmenu_method_call(GDBusConnection       *connection,
                     const gchar           *sender,
                     const gchar           *path,
                     const gchar           *iface,
                     const gchar           *method,
                     GVariant              *params,
                     GDBusMethodInvocation *invocation,
                     gpointer               user_data)
{
    GsniDBusMenu *self = user_data;

    if (g_str_equal(method, "GetLayout")) {
        gint parent_id, depth;
        g_autoptr(GVariant) prop_names_var = NULL;
        const gchar **prop_names = NULL;
        guint n_props = 0;

        g_variant_get(params, "(ii@as)", &parent_id, &depth,
                      &prop_names_var);
        prop_names = g_variant_get_strv(prop_names_var, NULL);
        n_props = g_variant_n_children(prop_names_var);

        g_autoptr(GVariant) layout = build_layout_recursive(
            self, self->menu_model, (guint32)parent_id, depth,
            prop_names, n_props, TRUE);

        GVariant *def_layout = g_variant_new_parsed("(0, @a{sv} {}, @av [])");
        if (layout == NULL)
            layout = g_variant_ref_sink(def_layout);

        GVariant *reply = g_variant_new("(u@(ia{sv}av))", self->revision,
                                        layout);
        g_free(prop_names);
        g_dbus_method_invocation_return_value(invocation, reply);

    } else if (g_str_equal(method, "GetGroupProperties")) {
        /* Return empty — clients should refetch layout instead */
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("a(ia{sv})"));
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(a(ia{sv}))", &b));

    } else if (g_str_equal(method, "GetProperty")) {
        g_dbus_method_invocation_return_error(invocation,
            G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
            "Use GetLayout instead");

    } else if (g_str_equal(method, "Event")) {
        gint id;
        const gchar *event_id;
        guint timestamp;

        g_variant_get(params, "(i&s@vu)", &id, &event_id, NULL, &timestamp);

        if (g_str_equal(event_id, "clicked") && self->action_group) {
            /* Walk to the item and activate its action */
            g_autofree gchar *action = NULL;
            g_autoptr(GVariant) target = NULL;
            /* TODO: walk GMenuModel using decode_parent / decode_position */
        }

        g_dbus_method_invocation_return_value(invocation, NULL);

    } else if (g_str_equal(method, "EventGroup")) {
        /* Return all IDs as errors for simplicity */
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("ai"));
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(ai)", &b));

    } else if (g_str_equal(method, "AboutToShow")) {
        /* Always return FALSE — updates are async via LayoutUpdated */
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(b)", FALSE));

    } else if (g_str_equal(method, "AboutToShowGroup")) {
        GVariantBuilder up, err;
        g_variant_builder_init(&up, G_VARIANT_TYPE("ai"));
        g_variant_builder_init(&err, G_VARIANT_TYPE("ai"));
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(aiai)", &up, &err));

    } else {
        g_dbus_method_invocation_return_error(invocation,
            G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
            "Unknown method: %s", method);
    }
}

static GVariant *
dbusmenu_get_property(GDBusConnection *connection,
                      const gchar     *sender,
                      const gchar     *path,
                      const gchar     *iface,
                      const gchar     *property,
                      GError         **error,
                      gpointer         user_data)
{
    if (g_str_equal(property, "Version"))
        return g_variant_new_uint32(DBUSMENU_VERSION);

    if (g_str_equal(property, "TextDirection"))
        return g_variant_new_string("ltr");

    if (g_str_equal(property, "Status"))
        return g_variant_new_string("normal");

    if (g_str_equal(property, "IconThemePath"))
        return g_variant_new_strv(NULL, 0);

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                "Unknown property: %s", property);
    return NULL;
}

static const GDBusInterfaceVTable dbusmenu_vtable = {
    .method_call  = dbusmenu_method_call,
    .get_property = dbusmenu_get_property,
    .set_property = NULL,
};

/* --- Layout builder --- */

static gboolean
property_requested(const gchar *name, const gchar *const *props,
                   guint n_props)
{
    if (n_props == 0)
        return TRUE;

    for (guint i = 0; i < n_props; i++) {
        if (g_str_equal(name, props[i]))
            return TRUE;
    }
    return FALSE;
}

/*
 * Collect GMenuItem attributes at @position into a DBusMenu property dict.
 */
static void
collect_item_properties(GMenuModel *model, gint position,
                        GActionGroup *actions,
                        const gchar *const *props, guint n_props,
                        gboolean is_separator,
                        gboolean has_submenu,
                        GVariantBuilder *out)
{
    g_variant_builder_init(out, G_VARIANT_TYPE("a{sv}"));

    if (is_separator) {
        if (property_requested("type", props, n_props))
            g_variant_builder_add(out, "{sv}", "type",
                g_variant_new_variant(g_variant_new_string("separator")));
        if (property_requested("visible", props, n_props))
            g_variant_builder_add(out, "{sv}", "visible",
                g_variant_new_variant(g_variant_new_boolean(TRUE)));
        if (property_requested("enabled", props, n_props))
            g_variant_builder_add(out, "{sv}", "enabled",
                g_variant_new_variant(g_variant_new_boolean(FALSE)));
        return;
    }

    if (property_requested("type", props, n_props))
        g_variant_builder_add(out, "{sv}", "type",
            g_variant_new_variant(g_variant_new_string("standard")));

    if (property_requested("visible", props, n_props))
        g_variant_builder_add(out, "{sv}", "visible",
            g_variant_new_variant(g_variant_new_boolean(TRUE)));

    if (property_requested("enabled", props, n_props)) {
        gboolean enabled = TRUE;
        g_autofree gchar *action_name = NULL;
        GMenuAttributeIter *iter = g_menu_model_iterate_item_attributes(
            model, position);
        const gchar *attr;
        GVariant *val;
        while (g_menu_attribute_iter_get_next(iter, &attr, &val)) {
            if (g_str_equal(attr, "action")) {
                action_name = g_variant_dup_string(val, NULL);
            }
            g_variant_unref(val);
        }
        g_object_unref(iter);

        if (action_name && actions &&
            g_action_group_has_action(actions, action_name)) {
            enabled = g_action_group_get_action_enabled(actions,
                                                        action_name);
        }
        g_variant_builder_add(out, "{sv}", "enabled",
            g_variant_new_variant(g_variant_new_boolean(enabled)));
    }

    if (property_requested("label", props, n_props)) {
        g_autofree gchar *label = NULL;
        GMenuAttributeIter *iter = g_menu_model_iterate_item_attributes(
            model, position);
        const gchar *attr;
        GVariant *val;
        while (g_menu_attribute_iter_get_next(iter, &attr, &val)) {
            if (g_str_equal(attr, "label"))
                label = g_variant_dup_string(val, NULL);
            g_variant_unref(val);
        }
        g_object_unref(iter);
        g_variant_builder_add(out, "{sv}", "label",
            g_variant_new_variant(
                g_variant_new_string(label ? label : "")));
    }

    if (has_submenu && property_requested("children-display", props,
                                          n_props))
        g_variant_builder_add(out, "{sv}", "children-display",
            g_variant_new_variant(g_variant_new_string("submenu")));

    /* Check for toggle state */
    g_autofree gchar *action = NULL;
    GMenuAttributeIter *iter2 = g_menu_model_iterate_item_attributes(
        model, position);
    const gchar *attr2;
    GVariant *val2;
    while (g_menu_attribute_iter_get_next(iter2, &attr2, &val2)) {
        if (g_str_equal(attr2, "action"))
            action = g_variant_dup_string(val2, NULL);
        g_variant_unref(val2);
    }
    g_object_unref(iter2);

    if (action && actions &&
        g_action_group_has_action(actions, action)) {
        GVariant *state = g_action_group_get_action_state(actions,
                                                          action);
        if (state && g_variant_is_of_type(state, G_VARIANT_TYPE_BOOLEAN)) {
            if (property_requested("toggle-type", props, n_props))
                g_variant_builder_add(out, "{sv}", "toggle-type",
                    g_variant_new_variant(g_variant_new_string("checkmark")));
            if (property_requested("toggle-state", props, n_props))
                g_variant_builder_add(out, "{sv}", "toggle-state",
                    g_variant_new_variant(g_variant_new_int32(
                        g_variant_get_boolean(state) ? 1 : 0)));
        }
        if (state)
            g_variant_unref(state);
    }
}

/*
 * Recursively build a (ia{sv}av) layout tuple from GMenuModel.
 */
static GVariant *
build_layout_recursive(GsniDBusMenu *self, GMenuModel *model,
                       guint32 parent_id, gint recurrence,
                       const gchar *const *props, guint n_props,
                       gboolean root_section)
{
    gint n_items = g_menu_model_get_n_items(model);
    GVariant *sub_children = NULL;
    GVariant *result = NULL;

    /* Root node properties */
    GVariantBuilder root_dict;
    g_variant_builder_init(&root_dict, G_VARIANT_TYPE("a{sv}"));

    if (parent_id == 0) {
        if (property_requested("children-display", props, n_props))
            g_variant_builder_add(&root_dict, "{sv}", "children-display",
                g_variant_new_variant(g_variant_new_string("submenu")));
        if (property_requested("visible", props, n_props))
            g_variant_builder_add(&root_dict, "{sv}", "visible",
                g_variant_new_variant(g_variant_new_boolean(TRUE)));
    }

    /* Build children array */
    GVariantBuilder children_arr;
    g_variant_builder_init(&children_arr, G_VARIANT_TYPE("av"));

    if (recurrence != 0 && n_items > 0) {
        gint child_recurse = (recurrence > 0) ? recurrence - 1 : -1;

        for (gint i = 0; i < n_items; i++) {
            GMenuModel *section = g_menu_model_get_item_link(model, i,
                G_MENU_LINK_SECTION);

            if (section) {
                g_object_unref(section);
                /* skip sections for now — they're flattened */
                continue;
            }

            GMenuModel *submenu = g_menu_model_get_item_link(model, i,
                G_MENU_LINK_SUBMENU);
            guint32 child_id = encode_id(parent_id, i);

            GVariantBuilder item_dict;
            collect_item_properties(model, i, self->action_group,
                                    props, n_props, FALSE,
                                    submenu != NULL, &item_dict);
            GVariant *item_props = g_variant_builder_end(&item_dict);
            g_variant_ref_sink(item_props);

            GVariantBuilder node;
            g_variant_builder_init(&node, G_VARIANT_TYPE("(ia{sv}av)"));
            g_variant_builder_add(&node, "i", child_id);
            g_variant_builder_add_value(&node, item_props);

            if (submenu) {
                sub_children = build_layout_recursive(self, submenu,
                    child_id, child_recurse, props, n_props, FALSE);
                g_object_unref(submenu);

                if (sub_children) {
                    GVariant *sc = g_variant_get_child_value(sub_children, 2);
                    g_variant_builder_add_value(&node, sc);
                    g_variant_unref(sc);
                    g_variant_unref(sub_children);
                } else {
                    g_variant_builder_add(&node, "av", NULL);
                }
            } else {
                g_variant_builder_add(&node, "av", NULL);
            }

            GVariant *node_var = g_variant_builder_end(&node);
            GVariant *wrapped = g_variant_new_variant(node_var);
            g_variant_builder_add_value(&children_arr, wrapped);
        }
    }

    GVariant *children = g_variant_builder_end(&children_arr);

    GVariantBuilder tuple;
    g_variant_builder_init(&tuple, G_VARIANT_TYPE("(ia{sv}av)"));
    g_variant_builder_add(&tuple, "i", parent_id);
    g_variant_builder_add_value(&tuple, g_variant_builder_end(&root_dict));
    g_variant_builder_add_value(&tuple, children);

    result = g_variant_builder_end(&tuple);

    return result;
}

/* --- Idle coalescing callbacks --- */

static gboolean
layout_update_idle_cb(gpointer user_data)
{
    GsniDBusMenu *self = user_data;

    g_dbus_connection_emit_signal(self->connection, NULL,
        self->object_path, DBUSMENU_IFACE, "LayoutUpdated",
        g_variant_new("(ui)", self->revision, 0), NULL);

    self->layout_idle_id = 0;
    return G_SOURCE_REMOVE;
}

static void
schedule_layout_update(GsniDBusMenu *self)
{
    self->revision++;

    if (self->layout_idle_id == 0)
        self->layout_idle_id = g_idle_add(layout_update_idle_cb, self);
}

/* --- GMenuModel signal handlers --- */

static void
on_menu_items_changed(GMenuModel *model, gint position,
                      gint removed, gint added, gpointer user_data)
{
    schedule_layout_update(user_data);
}

static void
on_action_state_changed(GActionGroup *group, const gchar *name,
                        GVariant *state, gpointer user_data)
{
    /* Booleans affect toggle-state — schedule property update */
    if (g_variant_is_of_type(state, G_VARIANT_TYPE_BOOLEAN)) {
        /* TODO: find item IDs that reference this action and queue */
    }
}

static void
on_action_enabled_changed(GActionGroup *group, const gchar *name,
                          gboolean enabled, gpointer user_data)
{
    /* TODO: find item IDs and queue "enabled" property update */
}

/* --- Public API --- */

static void
ensure_iface_info(void)
{
    if (dbusmenu_iface_info != NULL)
        return;

    GError *error = NULL;
    GDBusNodeInfo *node = g_dbus_node_info_new_for_xml(dbusmenu_xml,
                                                       &error);
    if (node == NULL) {
        g_error("Failed to parse dbusmenu XML: %s", error->message);
        g_error_free(error);
        return;
    }

    dbusmenu_iface_info = g_dbus_node_info_lookup_interface(node,
        DBUSMENU_IFACE);
    if (dbusmenu_iface_info == NULL)
        g_error("Missing %s in dbusmenu XML", DBUSMENU_IFACE);

    g_dbus_interface_info_ref(dbusmenu_iface_info);
    g_dbus_node_info_unref(node);
}

GsniDBusMenu *
gsni_dbusmenu_new(GDBusConnection *connection,
                  const gchar     *object_path,
                  GMenuModel      *menu,
                  GActionGroup    *actions)
{
    GsniDBusMenu *self;

    g_return_val_if_fail(G_IS_DBUS_CONNECTION(connection), NULL);
    g_return_val_if_fail(object_path != NULL, NULL);
    g_return_val_if_fail(G_IS_MENU_MODEL(menu), NULL);

    self = g_new0(GsniDBusMenu, 1);
    self->connection = connection;
    self->object_path = g_strdup(object_path);
    self->menu_model = g_object_ref(menu);
    self->action_group = actions ? g_object_ref(actions) : NULL;
    self->revision = 1;

    return self;
}

gboolean
gsni_dbusmenu_export(GsniDBusMenu *self, GError **error)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    ensure_iface_info();

    self->reg_id = g_dbus_connection_register_object(
        self->connection, self->object_path,
        dbusmenu_iface_info, &dbusmenu_vtable,
        self, NULL, error);

    if (self->reg_id == 0)
        return FALSE;

    /* Watch for menu model changes */
    self->menu_changed_id = g_signal_connect(
        self->menu_model, "items-changed",
        G_CALLBACK(on_menu_items_changed), self);

    /* Watch for action state changes */
    if (self->action_group) {
        self->action_state_id = g_signal_connect(
            self->action_group, "action-state-changed",
            G_CALLBACK(on_action_state_changed), self);
        self->action_enabled_id = g_signal_connect(
            self->action_group, "action-enabled-changed",
            G_CALLBACK(on_action_enabled_changed), self);
    }

    return TRUE;
}

void
gsni_dbusmenu_unexport(GsniDBusMenu *self)
{
    g_return_if_fail(self != NULL);

    if (self->layout_idle_id) {
        g_source_remove(self->layout_idle_id);
        self->layout_idle_id = 0;
    }

    if (self->menu_changed_id) {
        g_signal_handler_disconnect(self->menu_model,
                                    self->menu_changed_id);
        self->menu_changed_id = 0;
    }

    if (self->action_state_id) {
        g_signal_handler_disconnect(self->action_group,
                                    self->action_state_id);
        self->action_state_id = 0;
    }

    if (self->action_enabled_id) {
        g_signal_handler_disconnect(self->action_group,
                                    self->action_enabled_id);
        self->action_enabled_id = 0;
    }

    if (self->reg_id) {
        g_dbus_connection_unregister_object(self->connection,
                                            self->reg_id);
        self->reg_id = 0;
    }
}

void
gsni_dbusmenu_free(GsniDBusMenu *self)
{
    g_return_if_fail(self != NULL);

    g_free(self->object_path);
    g_clear_object(&self->menu_model);
    g_clear_object(&self->action_group);
    g_free(self);
}
