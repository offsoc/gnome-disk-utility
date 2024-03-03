/*
 * Copyright (C) 2017 Kai Lüke
 *
 * Licensed under GPL version 2 or later.
 *
 * Author: Kai Lüke <kailueke@riseup.net>
 */

#include "config.h"

#include <glib/gi18n.h>

#include "gdu-create-filesystem-page.h"

struct _GduCreateFilesystemPage
{
  GtkGrid parent_instance;
};

typedef struct _GduCreateFilesystemPagePrivate GduCreateFilesystemPagePrivate;

struct _GduCreateFilesystemPagePrivate
{
  GtkEntry *name_entry;
  GtkSwitch *erase_switch;
  GtkCheckButton *internal_checkbutton;
  GtkCheckButton *internal_encrypt_checkbutton;
  GtkCheckButton *windows_checkbutton;
  GtkCheckButton *all_checkbutton;
  GtkCheckButton *other_checkbutton;
};

enum
{
  PROP_0,
  PROP_COMPLETE
};

G_DEFINE_TYPE_WITH_PRIVATE (GduCreateFilesystemPage, gdu_create_filesystem_page, GTK_TYPE_GRID);

static void
gdu_create_filesystem_page_init (GduCreateFilesystemPage *page)
{
  gtk_widget_init_template (GTK_WIDGET (page));
}

static void
gdu_create_filesystem_page_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_COMPLETE:
      g_value_set_boolean (value, TRUE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gdu_create_filesystem_page_class_init (GduCreateFilesystemPageClass *class)
{
  GObjectClass *gobject_class;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/DiskUtility/ui/gdu-create-filesystem-page.ui");
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, name_entry);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, erase_switch);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, internal_checkbutton);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, internal_encrypt_checkbutton);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, windows_checkbutton);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, all_checkbutton);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), GduCreateFilesystemPage, other_checkbutton);

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->get_property = gdu_create_filesystem_page_get_property;
  g_object_class_install_property (gobject_class, PROP_COMPLETE,
                                   g_param_spec_boolean ("complete", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));
}

const gchar *
gdu_create_filesystem_page_get_name (GduCreateFilesystemPage *page)
{
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  return gtk_editable_get_text (GTK_EDITABLE (priv->name_entry));
}

gboolean
gdu_create_filesystem_page_is_other (GduCreateFilesystemPage *page)
{
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->other_checkbutton));
}

const gchar *
gdu_create_filesystem_page_get_fs (GduCreateFilesystemPage *page)
{
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->internal_checkbutton)))
    return "ext4";
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->windows_checkbutton)))
    return "ntfs";
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->all_checkbutton)))
    return "vfat";
  else
    return NULL;
}

gboolean
gdu_create_filesystem_page_is_encrypted (GduCreateFilesystemPage *page)
{
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->internal_checkbutton)) &&
         gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->internal_encrypt_checkbutton));
}

const gchar *
gdu_create_filesystem_page_get_erase (GduCreateFilesystemPage *page)
{
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  if (gtk_switch_get_active (priv->erase_switch))
    return "zero";
  else /* TODO: "ata-secure-erase", "ata-secure-erase-enhanced" */
    return NULL;
}

static void
on_fs_name_changed (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  GduCreateFilesystemPage *page = GDU_CREATE_FILESYSTEM_PAGE (user_data);
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  _gtk_entry_buffer_truncate_bytes (gtk_entry_get_buffer (priv->name_entry),
                                    gdu_utils_get_max_label_length (gdu_create_filesystem_page_get_fs (page)));
}

static void
on_fs_type_changed (GtkToggleButton *object, gpointer user_data)
{
  GduCreateFilesystemPage *page = GDU_CREATE_FILESYSTEM_PAGE (user_data);
  GduCreateFilesystemPagePrivate *priv;

  priv = gdu_create_filesystem_page_get_instance_private (page);

  _gtk_entry_buffer_truncate_bytes (gtk_entry_get_buffer (priv->name_entry),
                                    gdu_utils_get_max_label_length (gdu_create_filesystem_page_get_fs (page)));
  g_object_notify (G_OBJECT (page), "complete");
}

GduCreateFilesystemPage *
gdu_create_filesystem_page_new (UDisksClient *client, UDisksDrive *drive)
{
  GduCreateFilesystemPage *page;
  GduCreateFilesystemPagePrivate *priv;
  gchar *missing_util = NULL;

  page = g_object_new (GDU_TYPE_CREATE_FILESYSTEM_PAGE, NULL);
  priv = gdu_create_filesystem_page_get_instance_private (page);
  g_signal_connect (priv->name_entry, "notify::text", G_CALLBACK (on_fs_name_changed), page);
  g_signal_connect (priv->internal_encrypt_checkbutton, "toggled", G_CALLBACK (on_fs_type_changed), page);
  g_signal_connect (priv->internal_checkbutton, "toggled", G_CALLBACK (on_fs_type_changed), page);
  g_signal_connect (priv->windows_checkbutton, "toggled", G_CALLBACK (on_fs_type_changed), page);
  g_signal_connect (priv->all_checkbutton, "toggled", G_CALLBACK (on_fs_type_changed), page);
  g_signal_connect (priv->other_checkbutton, "toggled", G_CALLBACK (on_fs_type_changed), page);

  g_object_bind_property (priv->internal_checkbutton, "active", priv->internal_encrypt_checkbutton, "sensitive", G_BINDING_SYNC_CREATE);

  /* Default to FAT or NTFS for removable drives... */
  if (drive != NULL && udisks_drive_get_removable (drive))
    {
      /* default FAT for flash and disks/media smaller than 20G (assumed to be flash cards) */
      if (gdu_utils_can_format (client, "vfat", FALSE, NULL) &&
          (gdu_utils_is_flash (drive) ||
          udisks_drive_get_size (drive) < 20UL * 1000UL*1000UL*1000UL ||
          !gdu_utils_can_format (client, "ntfs", FALSE, NULL))
          )
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->all_checkbutton), TRUE);
        }
      else if (gdu_utils_can_format (client, "ntfs", FALSE, NULL))
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->windows_checkbutton), TRUE);
        }
    }


  if (!gdu_utils_can_format (client, "ntfs", FALSE, &missing_util))
    {
      gchar *s;

      gtk_widget_set_sensitive (GTK_WIDGET (priv->windows_checkbutton), FALSE);
      s = g_strdup_printf (_("The utility %s is missing."), missing_util);
      gtk_widget_set_tooltip_text (GTK_WIDGET (priv->windows_checkbutton), s);

      g_free (s);
    }
  g_free (missing_util);

  if (!gdu_utils_can_format (client, "vfat", FALSE, &missing_util))
    {
      gchar *s;

      gtk_widget_set_sensitive (GTK_WIDGET (priv->all_checkbutton), FALSE);
      s = g_strdup_printf (_("The utility %s is missing."), missing_util);
      gtk_widget_set_tooltip_text (GTK_WIDGET (priv->all_checkbutton), s);

      g_free (s);
    }
  g_free (missing_util);

  return page;
}
