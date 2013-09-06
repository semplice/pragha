/*************************************************************************/
/* Copyright (C) 2012-2013 matias <mati86dl@gmail.com>                   */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <gudev/gudev.h>

#include "pragha.h"
#include "pragha-devices.h"
#include "pragha-utils.h"
#include "pragha-debug.h"

struct _PraghaDevices {
	struct con_win *cwin;
	GUdevClient *gudev_client;
	GUdevDevice *device;
	guint64 bus_hooked;
	guint64 device_hooked;
};

static const gchar * gudev_subsystems[] =
{
	"block",
	"usb",
	NULL,
};

/* Headers. */

static void pragha_gudev_clear_hook_devices(PraghaDevices *devices);

/*
 * Generic dialog to get some action to responce to udev events.
 */

static gint
pragha_gudev_show_dialog (const gchar *title, const gchar *icon,
                          const gchar *primary_text, const gchar *secondary_text,
                          const gchar *first_button_text, gint first_button_response)
{
	GtkWidget *dialog;
	GtkWidget *image;
	gint response = PRAGHA_DEVICE_RESPONSE_NONE;

	dialog = gtk_message_dialog_new (NULL,
	                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_QUESTION,
	                                 GTK_BUTTONS_NONE,
	                                 NULL);

	if (title != NULL)
		gtk_window_set_title (GTK_WINDOW (dialog), title);

	gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), primary_text);

	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Ignore"), PRAGHA_DEVICE_RESPONSE_NONE);
	gtk_dialog_add_button (GTK_DIALOG (dialog), first_button_text, first_button_response);

	if(icon != NULL) {
		image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_DIALOG);
		gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG (dialog), image);
	}
	if (secondary_text != NULL)
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog), secondary_text);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), PRAGHA_DEVICE_RESPONSE_NONE);

	gtk_widget_show_all(dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return response;
}

/*
 * Some functions to mount block devices.
 */

/* Decode the ID_FS_LABEL_ENC of block device.
 * Extentions copy of Thunar-volman code.
 * http://git.xfce.org/xfce/thunar-volman/tree/thunar-volman/tvm-gio-extensions.c */

static gchar *
tvm_notify_decode (const gchar *str)
{
	GString     *string;
	const gchar *p;
	gchar       *result;
	gchar        decoded_c;

	if (str == NULL)
		return NULL;

	if (!g_utf8_validate (str, -1, NULL))
		return NULL;

	string = g_string_new (NULL);

	for (p = str; p != NULL && *p != '\0'; ++p) {
		if (*p == '\\' && p[1] == 'x' && g_ascii_isalnum (p[2]) && g_ascii_isalnum (p[3])) {
			decoded_c = (g_ascii_xdigit_value (p[2]) << 4) | g_ascii_xdigit_value (p[3]);
			g_string_append_c (string, decoded_c);
			p = p + 3;
		}
		else
			g_string_append_c (string, *p);
	}

	result = string->str;
	g_string_free (string, FALSE);

	return result;
}

static GVolume *
tvm_g_volume_monitor_get_volume_for_kind (GVolumeMonitor *monitor,
                                          const gchar    *kind,
                                          const gchar    *identifier)
{
	GVolume *volume = NULL;
	GList   *volumes;
	GList   *lp;
	gchar   *value;

	g_return_val_if_fail (G_IS_VOLUME_MONITOR (monitor), NULL);
	g_return_val_if_fail (kind != NULL && *kind != '\0', NULL);
	g_return_val_if_fail (identifier != NULL && *identifier != '\0', NULL);

	volumes = g_volume_monitor_get_volumes (monitor);

	for (lp = volumes; volume == NULL && lp != NULL; lp = lp->next) {
		value = g_volume_get_identifier (lp->data, kind);
		if (value == NULL)
			continue;
		if (g_strcmp0 (value, identifier) == 0)
			volume = g_object_ref (lp->data);
		g_free (value);
	}
	g_list_foreach (volumes, (GFunc)g_object_unref, NULL);
	g_list_free (volumes);

	return volume;
}

/* The next funtions allow to handle block devicess.
 * The most of code is inspired in:
 * http://git.xfce.org/xfce/thunar-volman/tree/thunar-volman/tvm-block-devices.c */

static void
pragha_block_device_mounted (GUdevDevice *device, GMount *mount, GError **error)
{
	const gchar *volume_name;
	gchar       *decoded_name;
	gchar       *message;
	GFile       *mount_point;
	gchar       *mount_path;
	gint         response;

	g_return_if_fail (G_IS_MOUNT (mount));
	g_return_if_fail (error == NULL || *error == NULL);

	volume_name = g_udev_device_get_property (device, "ID_FS_LABEL_ENC");
	decoded_name = tvm_notify_decode (volume_name);
  
	if (decoded_name != NULL)
		message = g_strdup_printf (_("The volume \"%s\" was mounted automatically"), decoded_name);
	else
		message = g_strdup_printf (_("The inserted volume was mounted automatically"));

	response = pragha_gudev_show_dialog (_("Audio/Data CD"), "gnome-fs-executable",
	                                     message, NULL,
	                                     _("Open folder"), PRAGHA_DEVICE_RESPONSE_BROWSE);
	switch (response)
	{
		case PRAGHA_DEVICE_RESPONSE_BROWSE:
			mount_point = g_mount_get_root (mount);
			mount_path = g_file_get_path (mount_point);

			open_url (mount_path, NULL);

			g_object_unref (mount_point);
			break;
		case PRAGHA_DEVICE_RESPONSE_NONE:
		default:
			break;
	}

	g_free (decoded_name);
	g_free (message);
}

static void
pragha_block_device_mount_finish (GVolume *volume, GAsyncResult *result, PraghaDevices *devices)
{
	GMount *mount;
	GError *error = NULL;
  
	g_return_if_fail (G_IS_VOLUME (volume));
	g_return_if_fail (G_IS_ASYNC_RESULT (result));

	/* finish mounting the volume */
	if (g_volume_mount_finish (volume, result, &error)) {
		/* get the moint point of the volume */
		mount = g_volume_get_mount (volume);

		if (mount != NULL) {
			pragha_block_device_mounted (devices->device, mount, &error);
			g_object_unref (mount);
		}
	}

	if(error != NULL) {
		// TODO: Check G_IO_ERROR_ALREADY_MOUNTED and ignore it..
		pragha_gudev_clear_hook_devices(devices);
	}

	g_object_unref (volume);
}

static gboolean
pragha_block_device_mount (gpointer data)
{
	GVolumeMonitor  *monitor;
	GMountOperation *mount_operation;
	GVolume         *volume;

	PraghaDevices *devices = data;

	monitor = g_volume_monitor_get ();

	/* determine the GVolume corresponding to the udev devices */
	volume = tvm_g_volume_monitor_get_volume_for_kind (monitor,
	                                                   G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE,
	                                                   g_udev_device_get_device_file (devices->device));
	g_object_unref (monitor);

	/* check if we have a volume */
	if (volume != NULL) {
		if (g_volume_can_mount (volume)) {
			/* try to mount the volume asynchronously */
			mount_operation = gtk_mount_operation_new (NULL);
			g_volume_mount (volume, G_MOUNT_MOUNT_NONE, mount_operation,
			                NULL, (GAsyncReadyCallback) pragha_block_device_mount_finish,
			                devices);
			g_object_unref (mount_operation);
		}
	}

	return FALSE;
}

/*
 * Check the type of device that listens udev to act accordingly.
 */

static gint
pragha_gudev_get_device_type (GUdevDevice *device)
{
	const gchar *devtype;
	const gchar *id_type;
	const gchar *id_fs_usage;
	gboolean     is_cdrom;
	gboolean     is_partition;
	gboolean     is_volume;
	guint64      audio_tracks = 0;
	guint64      data_tracks = 0;
	guint64      is_mtpdevice = 0;

	/* collect general devices information */

	id_type = g_udev_device_get_property (device, "ID_TYPE");

	is_cdrom = (g_strcmp0 (id_type, "cd") == 0);
	if (is_cdrom) {
		/* silently ignore CD drives without media */
		if (g_udev_device_get_property_as_boolean (device, "ID_CDROM_MEDIA")) {

			audio_tracks = g_udev_device_get_property_as_uint64 (device, "ID_CDROM_MEDIA_TRACK_COUNT_AUDIO");
			data_tracks = g_udev_device_get_property_as_uint64 (device, "ID_CDROM_MEDIA_TRACK_COUNT_DATA");

			if (audio_tracks > 0)
				return PRAGHA_DEVICE_AUDIO_CD;
		}
	}

	devtype = g_udev_device_get_property (device, "DEVTYPE");
	id_fs_usage = g_udev_device_get_property (device, "ID_FS_USAGE");

	is_partition = (g_strcmp0 (devtype, "partition") == 0);
	is_volume = (g_strcmp0 (devtype, "disk") == 0) && (g_strcmp0 (id_fs_usage, "filesystem") == 0);

	if (is_partition || is_volume || data_tracks)
		return PRAGHA_DEVICE_MOUNTABLE;

	is_mtpdevice = g_udev_device_get_property_as_uint64 (device, "ID_MTP_DEVICE");
	if (is_mtpdevice)
		return PRAGHA_DEVICE_MTP;

	return PRAGHA_DEVICE_UNKNOWN;
}

static void
pragha_devices_moutable_added (PraghaDevices *devices, GUdevDevice *device)
{
	guint64 busnum = 0;
	guint64 devnum = 0;

	if (devices->bus_hooked != 0 &&
	    devices->device_hooked != 0)
		return;

	busnum = g_udev_device_get_property_as_uint64(device, "BUSNUM");
	devnum = g_udev_device_get_property_as_uint64(device, "DEVNUM");

	devices->device = g_object_ref (device);
	devices->bus_hooked = busnum;
	devices->device_hooked = devnum;

	CDEBUG(DBG_INFO, "Hook a new mountable device, Bus: %ld, Dev: %ld", devices->bus_hooked, devices->device_hooked);

	/*
	 * HACK: We're listening udev. Then wait 5 seconds, to ensure that GVolume also detects the device.
	 */
	g_timeout_add_seconds(5, pragha_block_device_mount, devices);
}

static void
pragha_devices_audio_cd_added (PraghaDevices *devices)
{
	gint response;
	response = pragha_gudev_show_dialog (_("Audio/Data CD"), "gnome-dev-cdrom-audio",
	                                     _("Was inserted an Audio Cd."), NULL,
	                                     _("Add Audio _CD"), PRAGHA_DEVICE_RESPONSE_PLAY);
	switch (response)
	{
		case PRAGHA_DEVICE_RESPONSE_PLAY:
			add_audio_cd (devices->cwin);
			break;
		case PRAGHA_DEVICE_RESPONSE_NONE:
		default:
			break;
	}
}

static void
pragha_devices_mtp_added (PraghaDevices *devices, GUdevDevice *device)
{
	guint64 busnum = 0;
	guint64 devnum = 0;

	if (devices->bus_hooked != 0 &&
	    devices->device_hooked != 0)
		return;

	busnum = g_udev_device_get_property_as_uint64(device, "BUSNUM");
	devnum = g_udev_device_get_property_as_uint64(device, "DEVNUM");

	devices->device = g_object_ref (device);
	devices->bus_hooked = busnum;
	devices->device_hooked = devnum;

	CDEBUG(DBG_INFO, "Hook a new MTP device, Bus: %ld, Dev: %ld", devices->bus_hooked, devices->device_hooked);
}

static void
pragha_gudev_clear_hook_devices (PraghaDevices *devices)
{
	CDEBUG(DBG_INFO, "Clear hooked device, Bus: %ld, Dev: %ld", devices->bus_hooked, devices->device_hooked);

	g_object_unref (devices->device);

	devices->bus_hooked = 0;
	devices->device_hooked = 0;
}

/* Functions that manage to "add" "change" and "remove" devices events. */

static void
pragha_gudev_device_added (PraghaDevices *devices, GUdevDevice *device)
{
	gint device_type = 0;

	device_type = pragha_gudev_get_device_type (device);

	switch (device_type) {
		case PRAGHA_DEVICE_MOUNTABLE:
			pragha_devices_moutable_added (devices, device);
			break;
		case PRAGHA_DEVICE_AUDIO_CD:
			pragha_devices_audio_cd_added (devices);
			break;
		case PRAGHA_DEVICE_MTP:
			pragha_devices_mtp_added (devices, device);
		case PRAGHA_DEVICE_UNKNOWN:
		default:
			break;
	}
}

static void
pragha_gudev_device_changed (PraghaDevices *devices, GUdevDevice *device)
{
	gint device_type = 0;

	device_type = pragha_gudev_get_device_type (device);

	if (device_type == PRAGHA_DEVICE_AUDIO_CD)
		pragha_devices_audio_cd_added (devices);
}

static void
pragha_gudev_device_removed (PraghaDevices *devices, GUdevDevice *device)
{
	guint64 busnum = 0;
	guint64 devnum = 0;

	if (devices->bus_hooked == 0 &&
	    devices->device_hooked == 0)
		return;

	busnum = g_udev_device_get_property_as_uint64 (device, "BUSNUM");
	devnum = g_udev_device_get_property_as_uint64 (device, "DEVNUM");

	if (devices->bus_hooked == busnum &&
	    devices->device_hooked == devnum) {
		pragha_gudev_clear_hook_devices (devices);
	}
}

/* Main devices functions that listen udev events. */

static void
gudev_uevent_cb(GUdevClient *client, const char *action, GUdevDevice *device, PraghaDevices *devices)
{
	if (g_str_equal(action, "add")) {
		pragha_gudev_device_added (devices, device);
	}
	else if (g_str_equal(action, "change")) {
		pragha_gudev_device_changed (devices, device);
	}
	else if (g_str_equal (action, "remove")) {
		pragha_gudev_device_removed (devices, device);
	}
}

/* Init gudev subsysten, and listen events. */

void
pragha_devices_free(PraghaDevices *devices)
{
	if(devices->bus_hooked != 0 &&
	   devices->device_hooked != 0)
		pragha_gudev_clear_hook_devices (devices);

	g_object_unref(devices->gudev_client);

	g_slice_free (PraghaDevices, devices);
}

PraghaDevices *
pragha_devices_new (struct con_win *cwin)
{
	PraghaDevices *devices;

	devices = g_slice_new0(PraghaDevices);

	devices->cwin = cwin;
	devices->bus_hooked = 0;
	devices->device_hooked = 0;

	devices->gudev_client = g_udev_client_new(gudev_subsystems);

	g_signal_connect (devices->gudev_client, "uevent", G_CALLBACK(gudev_uevent_cb), devices);

	return devices;
}