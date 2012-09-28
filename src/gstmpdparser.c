/*
 * gstmpdparser.c - DASH MPD helper library
 * Copyright (C) 2012 STMicroelectronics
 * Authors:
 *   Gianluca Gennari <gennarone@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include "gstfragmented.h"
#include "gstmpdparser.h"

#define GST_CAT_DEFAULT fragmented_debug

/* Property parsing */
static gchar *gst_mpdparser_get_xml_prop_string (xmlNode * a_node, const gchar * property);
static gchar **gst_mpdparser_get_xml_prop_string_vector_type (xmlNode * a_node, const gchar * property);
static guint gst_mpdparser_get_xml_prop_unsigned_integer (xmlNode * a_node, const gchar * property, guint default_val);
static guint *gst_mpdparser_get_xml_prop_uint_vector_type (xmlNode * a_node, const gchar * property, guint * size);
static gdouble gst_mpdparser_get_xml_prop_double (xmlNode * a_node, const gchar * property);
static gboolean gst_mpdparser_get_xml_prop_boolean (xmlNode * a_node, const gchar * property);
static GstMPDFileType gst_mpdparser_get_xml_prop_type (xmlNode * a_node, const gchar * property);
static GstSAPType gst_mpdparser_get_xml_prop_SAP_type (xmlNode * a_node, const gchar * property);
static GstRange *gst_mpdparser_get_xml_prop_range (xmlNode * a_node, const gchar * property);
static GstRatio *gst_mpdparser_get_xml_prop_ratio (xmlNode * a_node, const gchar * property);
static GstFrameRate *gst_mpdparser_get_xml_prop_framerate (xmlNode * a_node, const gchar * property);
static GstConditionalUintType *gst_mpdparser_get_xml_prop_cond_uint (xmlNode * a_node, const gchar * property);
static GstDateTime *gst_mpdparser_get_xml_prop_dateTime (xmlNode * a_node, const gchar * property);
static gint64 gst_mpdparser_get_xml_prop_duration (xmlNode * a_node, const gchar * property);
static gchar *gst_mpdparser_get_xml_node_content (xmlNode * a_node);
static gchar *gst_mpdparser_get_xml_node_namespace (xmlNode * a_node, const gchar * prefix);

/* XML node parsing */
static void gst_mpdparser_parse_baseURL_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_descriptor_type_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_content_component_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_location_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_subrepresentation_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_segment_url_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_url_type_node (GstURLType ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_seg_base_type_ext (GstSegmentBaseType ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_s_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_segment_timeline_node (GstSegmentTimelineNode ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_mult_seg_base_type_ext (GstMultSegmentBaseType ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_segment_list_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_representation_base_type (GstRepresentationBaseType ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_representation_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_adaptation_set_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_subset_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_segment_template_node (GstSegmentTemplateNode ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_period_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_program_info_node (GstProgramInformationNode ** pointer, xmlNode * a_node);
static void gst_mpdparser_parse_metrics_range_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_metrics_node (GList ** list, xmlNode * a_node);
static void gst_mpdparser_parse_root_node (GstMpdClient * client, xmlNode *a_node);

/* Helper functions */
static gint convert_to_millisecs (gint decimals, gint pos);
static int strncmp_ext (const char *s1, const char *s2);
static gchar *gst_mpdparser_parse_baseURL (GstMpdClient * client);
static gchar *gst_mpdparser_get_mediaURL (GstSegmentURLNode *segmentURL);
static gchar *gst_mpdparser_get_initializationURL (GstURLType *InitializationURL);
static gchar *gst_mpdparser_build_URL_from_template (const gchar *url_template, const gchar *id, guint number, guint bandwidth, guint time);

/* Period */
static GstPeriodNode *gst_mpdparser_get_next_period (GList *Periods, GstPeriodNode *prev_period);

/* Adaptation Set */
static GstAdaptationSetNode *gst_mpdparser_get_first_adapt_set (GList *AdaptationSets);
static GstAdaptationSetNode *gst_mpdparser_get_first_adapt_set_with_mimeType (GList *AdaptationSets, const gchar *mimeType);
static GstAdaptationSetNode *gst_mpdparser_get_adapt_set_with_mimeType_and_idx (GList *AdaptationSets, const gchar *mimeType, gint idx);
static GstAdaptationSetNode *gst_mpdparser_get_first_adapt_set_with_mimeType_and_lang (GList *AdaptationSets, const gchar *mimeType, const gchar *lang);

/* Representation */
static GstRepresentationNode *gst_mpdparser_get_lowest_representation (GList * Representations);
#if 0
static GstRepresentationNode *gst_mpdparser_get_highest_representation (GList * Representations);
static GstRepresentationNode *gst_mpdparser_get_representation_with_max_bandwidth (GList * Representations, gint max_bandwidth);
#endif
static GstSegmentBaseType *gst_mpdparser_get_segment_base (GstPeriodNode *Period, GstAdaptationSetNode * AdaptationSet, GstRepresentationNode * Representation);
static GstSegmentListNode *gst_mpdparser_get_first_segment_list (GList * SegmentList);

/* Memory management */
static void gst_mpdparser_free_mpd_node (GstMPDNode * mpd_node);
static void gst_mpdparser_free_prog_info_node (GstProgramInformationNode * prog_info_node);
static void gst_mpdparser_free_metrics_node (GstMetricsNode * metrics_node);
static void gst_mpdparser_free_metrics_range_node (GstMetricsRangeNode * metrics_range_node);
static void gst_mpdparser_free_period_node (GstPeriodNode * period_node);
static void gst_mpdparser_free_subset_node (GstSubsetNode * subset_node);
static void gst_mpdparser_free_segment_template_node (GstSegmentTemplateNode * segment_template_node);
static void gst_mpdparser_free_representation_base_type (GstRepresentationBaseType * representation_base);
static void gst_mpdparser_free_adaptation_set_node (GstAdaptationSetNode * adaptation_set_node);
static void gst_mpdparser_free_representation_node (GstRepresentationNode * representation_node);
static void gst_mpdparser_free_subrepresentation_node (GstSubRepresentationNode * subrep_node);
static void gst_mpdparser_free_s_node (GstSNode * s_node);
static void gst_mpdparser_free_segment_timeline_node (GstSegmentTimelineNode * seg_timeline);
static void gst_mpdparser_free_url_type_node (GstURLType * url_type_node);
static void gst_mpdparser_free_seg_base_type_ext (GstSegmentBaseType * seg_base_type);
static void gst_mpdparser_free_mult_seg_base_type_ext (GstMultSegmentBaseType * mult_seg_base_type);
static void gst_mpdparser_free_segment_list_node (GstSegmentListNode * segment_list_node);
static void gst_mpdparser_free_segment_url_node (GstSegmentURLNode * segment_url);
static void gst_mpdparser_free_base_url_node (GstBaseURL * base_url_node);
static void gst_mpdparser_free_descriptor_type_node (GstDescriptorType * descriptor_type);
static void gst_mpdparser_free_content_component_node (GstContentComponentNode * content_component_node);
static void gst_mpdparser_free_media_segment (GstMediaSegment * media_segment);
static void gst_mpdparser_free_active_stream (GstActiveStream * active_stream);

/* functions to parse node namespaces, content and properties */
static gchar *
gst_mpdparser_get_xml_prop_string (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    GST_LOG (" - %s: %s", property, prop_string);
  }

  return (gchar *) prop_string;
}

static gchar **
gst_mpdparser_get_xml_prop_string_vector_type (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  gchar **prop_string_vector = NULL;
  guint i = 0;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    prop_string_vector = g_strsplit ((gchar *) prop_string, " ", -1);
    if (!prop_string_vector) {
      GST_WARNING ("Scan of string vector property failed!");
      return NULL;
    }
    GST_LOG (" - %s:", property);
    while (prop_string_vector[i]) {
      GST_LOG ("    %s", prop_string_vector[i]);
      i++;
    }
    xmlFree (prop_string);
  }

  return prop_string_vector;
}

static guint
gst_mpdparser_get_xml_prop_unsigned_integer (xmlNode * a_node, const gchar * property, guint default_val)
{
  xmlChar *prop_string;
  guint prop_unsigned_integer = default_val;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    if (sscanf ((gchar *) prop_string, "%u", &prop_unsigned_integer)) {
      GST_LOG (" - %s: %u", property, prop_unsigned_integer);
    } else {
      GST_WARNING ("failed to parse unsigned integer property %s from xml string %s",
          property, prop_string);
    }
    xmlFree (prop_string);
  }

  return prop_unsigned_integer;
}

static guint *
gst_mpdparser_get_xml_prop_uint_vector_type (xmlNode * a_node, const gchar * property, guint * size)
{
  xmlChar *prop_string;
  gchar **str_vector;
  guint *prop_uint_vector = NULL, i;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    str_vector = g_strsplit ((gchar *) prop_string, " ", -1);
    if (!str_vector) {
      GST_WARNING ("Scan of uint vector property failed!");
      return NULL;
    }
    *size = g_strv_length (str_vector);
    prop_uint_vector = g_malloc (*size * sizeof (guint));
    if (!prop_uint_vector) {
      GST_WARNING ("Array allocation failed!");
    } else {
      GST_LOG (" - %s:", property);
      for (i = 0; i < *size; i++) {
        if (sscanf ((gchar *) str_vector[i], "%u", &prop_uint_vector[i])) {
          GST_LOG ("    %u", prop_uint_vector[i]);
        } else {
          GST_WARNING ("failed to parse uint vector type property %s from xml string %s",
              property, str_vector[i]);
        }
      }
    }
    xmlFree (prop_string);
    g_strfreev (str_vector);
  }

  return prop_uint_vector;
}

static gdouble
gst_mpdparser_get_xml_prop_double (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  gdouble prop_double = 0;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    if (sscanf ((gchar *) prop_string, "%lf", &prop_double)) {
      GST_LOG (" - %s: %lf", property, prop_double);
    } else {
      GST_WARNING ("failed to parse double property %s from xml string %s",
          property, prop_string);
    }
    xmlFree (prop_string);
  }

  return prop_double;
}

static gboolean
gst_mpdparser_get_xml_prop_boolean (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  gboolean prop_bool = FALSE;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    if (xmlStrcmp (prop_string, (xmlChar *) "false") == 0) {
      GST_LOG (" - %s: false", property);
    } else if (xmlStrcmp (prop_string, (xmlChar *) "true") == 0) {
      GST_LOG (" - %s: true", property);
      prop_bool = TRUE;
    } else {
      GST_WARNING ("failed to parse boolean property %s from xml string %s",
          property, prop_string);
    }
    xmlFree (prop_string);
  }

  return prop_bool;
}

static GstMPDFileType
gst_mpdparser_get_xml_prop_type (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  GstMPDFileType prop_type = GST_MPD_FILE_TYPE_STATIC;  /* default */

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    if (xmlStrcmp (prop_string, (xmlChar *) "OnDemand") == 0
        || xmlStrcmp (prop_string, (xmlChar *) "static") == 0) {
      GST_LOG (" - %s: static", property);
      prop_type = GST_MPD_FILE_TYPE_STATIC;
    } else if (xmlStrcmp (prop_string, (xmlChar *) "Live") == 0
        || xmlStrcmp (prop_string, (xmlChar *) "dynamic") == 0) {
      GST_LOG (" - %s: dynamic", property);
      prop_type = GST_MPD_FILE_TYPE_DYNAMIC;
    } else {
      GST_WARNING ("failed to parse MPD type property %s from xml string %s",
          property, prop_string);
    }
    xmlFree (prop_string);
  }

  return prop_type;
}

static GstSAPType
gst_mpdparser_get_xml_prop_SAP_type (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  guint prop_SAP_type = 0;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    if (sscanf ((gchar *) prop_string, "%u", &prop_SAP_type)
        && prop_SAP_type <= 6) {
      GST_LOG (" - %s: %u", property, prop_SAP_type);
    } else {
      GST_WARNING ("failed to parse unsigned integer property %s from xml string %s",
          property, prop_string);
    }
    xmlFree (prop_string);
  }

  return (GstSAPType) prop_SAP_type;
}

static GstRange *
gst_mpdparser_get_xml_prop_range (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  GstRange *prop_range = NULL;
  guint64 first_byte_pos = 0, last_byte_pos = 0;
  guint len, pos;
  gchar *str;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    len = xmlStrlen (prop_string);
    str = (gchar *) prop_string;
    GST_TRACE ("range: %s, len %d", str, len);

    /* read "-" */
    pos = strcspn (str, "-");
    if (pos >= len) {
      GST_TRACE ("pos %d >= len %d", pos, len);
      goto error;
    }
    /* read first_byte_pos */
    if (pos != 0) {
      if (sscanf (str, "%llu", &first_byte_pos) != 1) {
        goto error;
      }
    }
    /* read last_byte_pos */
    if (pos < (len - 1)) {
      if (sscanf (str + pos + 1, "%llu", &last_byte_pos) != 1) {
        goto error;
      }
    }
    /* malloc return data structure */
    prop_range = g_slice_new0 (GstRange);
    if (prop_range == NULL) {
      GST_WARNING ("Allocation of GstRange failed!");
      goto error;
    }
    prop_range->first_byte_pos = first_byte_pos;
    prop_range->last_byte_pos = last_byte_pos;
    GST_LOG (" - %s: %" G_GUINT64_FORMAT "-%" G_GUINT64_FORMAT,
        property, first_byte_pos, last_byte_pos);
    xmlFree (prop_string);
  }

  return prop_range;

error:
  GST_WARNING("failed to parse property %s from xml string %s", property,
      prop_string);
  return NULL;
}

static GstRatio *
gst_mpdparser_get_xml_prop_ratio (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  GstRatio *prop_ratio = NULL;
  guint num = 0, den = 1;
  guint len, pos;
  gchar *str;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    len = xmlStrlen (prop_string);
    str = (gchar *) prop_string;
    GST_TRACE ("ratio: %s, len %d", str, len);

    /* read ":" */
    pos = strcspn (str, ":");
    if (pos >= len) {
      GST_TRACE ("pos %d >= len %d", pos, len);
      goto error;
    }
    /* read num */
    if (pos != 0) {
      if (sscanf (str, "%u", &num) != 1) {
        goto error;
      }
    }
    /* read den */
    if (pos < (len - 1)) {
      if (sscanf (str + pos + 1, "%u", &den) != 1) {
        goto error;
      }
    }
    /* malloc return data structure */
    prop_ratio = g_slice_new0 (GstRatio);
    if (prop_ratio == NULL) {
      GST_WARNING ("Allocation of GstRatio failed!");
      goto error;
    }
    prop_ratio->num = num;
    prop_ratio->den = den;
    GST_LOG (" - %s: %u:%u", property, num, den);
    xmlFree (prop_string);
  }

  return prop_ratio;

error:
  GST_WARNING ("failed to parse property %s from xml string %s", property,
      prop_string);
  return NULL;
}

static GstFrameRate *
gst_mpdparser_get_xml_prop_framerate (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  GstFrameRate *prop_framerate = NULL;
  guint num = 0, den = 1;
  guint len, pos;
  gchar *str;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    len = xmlStrlen (prop_string);
    str = (gchar *) prop_string;
    GST_TRACE ("framerate: %s, len %d", str, len);

    /* read "/" if available */
    pos = strcspn (str, "/");
    /* read num */
    if (pos != 0) {
      if (sscanf (str, "%u", &num) != 1) {
        goto error;
      }
    }
    /* read den (if available) */
    if (pos < (len - 1)) {
      if (sscanf (str + pos + 1, "%u", &den) != 1) {
        goto error;
      }
    }
    /* alloc return data structure */
    prop_framerate = g_slice_new0 (GstFrameRate);
    if (prop_framerate == NULL) {
      GST_WARNING ("Allocation of GstFrameRate failed!");
      goto error;
    }
    prop_framerate->num = num;
    prop_framerate->den = den;
    if (den == 1)
      GST_LOG (" - %s: %u", property, num);
    else
      GST_LOG (" - %s: %u/%u", property, num, den);
    xmlFree (prop_string);
  }

  return prop_framerate;

error:
  GST_WARNING ("failed to parse property %s from xml string %s", property,
      prop_string);
  return NULL;
}

static GstConditionalUintType *
gst_mpdparser_get_xml_prop_cond_uint (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  GstConditionalUintType *prop_cond_uint = NULL;
  gchar *str;
  gboolean flag;
  guint val;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    str = (gchar *) prop_string;
    GST_TRACE ("conditional uint: %s", str);

    if (strcmp (str, "false") == 0) {
      flag = FALSE;
      val = 0;
    } else if (strcmp (str, "true") == 0) {
      flag = TRUE;
      val = 0;
    } else {
      flag = TRUE;
      if (sscanf (str, "%u", &val) != 1)
        goto error;
    }

    /* alloc return data structure */
    prop_cond_uint = g_slice_new0 (GstConditionalUintType);
    if (prop_cond_uint == NULL) {
      GST_WARNING ("Allocation of GstConditionalUintType failed!");
      goto error;
    }
    prop_cond_uint->flag = flag;
    prop_cond_uint->value = val;
    GST_LOG (" - %s: flag=%s val=%u", property,
        flag ? "true" : "false", val);
    xmlFree (prop_string);
  }

  return prop_cond_uint;

error:
  GST_WARNING ("failed to parse property %s from xml string %s", property,
      prop_string);
  return NULL;
}

/*
  DateTime Data Type

  The dateTime data type is used to specify a date and a time.

  The dateTime is specified in the following form "YYYY-MM-DDThh:mm:ss" where:

    * YYYY indicates the year
    * MM indicates the month
    * DD indicates the day
    * T indicates the start of the required time section
    * hh indicates the hour
    * mm indicates the minute
    * ss indicates the second

  Note: All components are required!
*/

static GstDateTime *
gst_mpdparser_get_xml_prop_dateTime (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  gchar *str;
  gint ret, len, pos;
  gint year, month, day, hour, minute, second;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    len = xmlStrlen (prop_string);
    str = (gchar *) prop_string;
    GST_TRACE ("dateTime: %s, len %d", str, len);
    /* parse year */
    ret = sscanf (str, "%d", &year);
    if (ret != 1)
      goto error;
    pos = strcspn (str, "-");
    str += (pos + 1);
    GST_TRACE (" - year %d", year);
    /* parse month */
    ret = sscanf (str, "%d", &month);
    if (ret != 1)
      goto error;
    pos = strcspn (str, "-");
    str += (pos + 1);
    GST_TRACE (" - month %d", month);
    /* parse day */
    ret = sscanf (str, "%d", &day);
    if (ret != 1)
      goto error;
    pos = strcspn (str, "T");
    str += (pos + 1);
    GST_TRACE (" - day %d", day);
    /* parse hour */
    ret = sscanf (str, "%d", &hour);
    if (ret != 1)
      goto error;
    pos = strcspn (str, ":");
    str += (pos + 1);
    GST_TRACE (" - hour %d", hour);
    /* parse minute */
    ret = sscanf (str, "%d", &minute);
    if (ret != 1)
      goto error;
    pos = strcspn (str, ":");
    str += (pos + 1);
    GST_TRACE (" - minute %d", minute);
    /* parse second */
    ret = sscanf (str, "%d", &second);
    if (ret != 1)
      goto error;
    GST_TRACE (" - second %d", second);

    GST_LOG (" - %s: %4d/%02d/%02d %02d:%02d:%02d", property,
        year, month, day, hour, minute, second);

    return gst_date_time_new (0, year, month, day, hour, minute, second);
  }

  return NULL;

error:
  GST_WARNING ("failed to parse property %s from xml string %s", property,
      prop_string);
  return NULL;
}

/*
  Duration Data Type

  The duration data type is used to specify a time interval.

  The time interval is specified in the following form "-PnYnMnDTnHnMnS" where:

    * - indicates the negative sign (optional)
    * P indicates the period (required)
    * nY indicates the number of years
    * nM indicates the number of months
    * nD indicates the number of days
    * T indicates the start of a time section (required if you are going to specify hours, minutes, or seconds)
    * nH indicates the number of hours
    * nM indicates the number of minutes
    * nS indicates the number of seconds
*/

/* this function computes decimals * 10 ^ (3 - pos) */
static gint
convert_to_millisecs (gint decimals, gint pos)
{
  gint num = 1, den = 1, i = 3 - pos;

  while (i < 0) {
    den *= 10;
    i++;
  }
  while (i > 0) {
    num *= 10;
    i--;
  }
  /* if i == 0 we have exactly 3 decimals and nothing to do */
  return decimals * num / den;
}

static gint64
gst_mpdparser_get_xml_prop_duration (xmlNode * a_node, const gchar * property)
{
  xmlChar *prop_string;
  gchar *str;
  gint64 prop_duration = -1;
  gint ret, read, len, pos, posT;
  gint years = 0, months = 0, days = 0, hours = 0, minutes = 0, seconds =
      0, decimals = 0;
  gint sign = 1;
  gboolean have_ms = FALSE;

  prop_string = xmlGetProp (a_node, (const xmlChar *) property);
  if (prop_string) {
    len = xmlStrlen (prop_string);
    str = (gchar *) prop_string;
    GST_TRACE ("duration: %s, len %d", str, len);
    /* read "-" for sign, if present */
    pos = strcspn (str, "-");
    if (pos < len) {            /* found "-" */
      if (pos != 0) {
        GST_WARNING ("sign \"-\" non at the beginning of the string");
        return -1;
      }
      GST_TRACE ("found - sign at the beginning");
      sign = -1;
      str++;
      len--;
    }
    /* read "P" for period */
    pos = strcspn (str, "P");
    if (pos != 0) {
      GST_WARNING ("P not found at the beginning of the string!");
      return -1;
    }
    str++;
    len--;
    /* read "T" for time (if present) */
    posT = strcspn (str, "T");
    len -= posT;
    if (posT > 0) {
      /* there is some room between P and T, so there must be a period section */
      /* read years, months, days */
      do {
        GST_TRACE ("parsing substring %s", str);
        pos = strcspn (str, "YMD");
        ret = sscanf (str, "%d", &read);
        if (ret != 1) {
          GST_WARNING ("can not read integer value from string %s!", str);
          return -1;
        }
        switch (str[pos]) {
          case 'Y':
            years = read;
            break;
          case 'M':
            months = read;
            break;
          case 'D':
            days = read;
            break;
          default:
            GST_WARNING ("unexpected char %c!", str[pos]);
            return -1;
            break;
        }
        GST_TRACE ("read number %d type %c", read, str[pos]);
        str += (pos + 1);
        posT -= (pos + 1);
      } while (posT > 0);

      GST_TRACE ("Y:M:D=%d:%d:%d", years, months, days);
    }
    /* read "T" for time (if present) */
    /* here T is at pos == 0 */
    str++;
    len--;
    pos = 0;
    if (pos < len) {
      /* T found, there is a time section */
      /* read hours, minutes, seconds, cents of second */
      do {
        GST_TRACE ("parsing substring %s", str);
        pos = strcspn (str, "HMS,.");
        ret = sscanf (str, "%d", &read);
        if (ret != 1) {
          GST_WARNING ("can not read integer value from string %s!", str);
          return -1;
        }
        switch (str[pos]) {
          case 'H':
            hours = read;
            break;
          case 'M':
            minutes = read;
            break;
          case 'S':
            if (have_ms) {
              /* we have read the decimal part of the seconds */
              decimals = convert_to_millisecs (read, pos);
              GST_TRACE ("decimal number %d (%d digits) -> %d ms", read, pos,
                  decimals);
            } else {
              /* no decimals */
              seconds = read;
            }
            break;
          case '.':
          case ',':
            /* we have read the integer part of a decimal number in seconds */
            seconds = read;
            have_ms = TRUE;
            break;
          default:
            GST_WARNING ("unexpected char %c!", str[pos]);
            return -1;
            break;
        }
        GST_TRACE ("read number %d type %c", read, str[pos]);
        str += pos + 1;
        len -= (pos + 1);
      } while (len > 0);

      GST_TRACE ("H:M:S.MS=%d:%d:%d.%03d", hours, minutes,
          seconds, decimals);
    }

    xmlFree (prop_string);
    prop_duration =
        sign * ((((((gint64) years * 365 + months * 30 + days) * 24 +
                    hours) * 60 + minutes) * 60 + seconds) * 1000 + decimals);
    GST_LOG (" - %s: %" G_GINT64_FORMAT, property,
        prop_duration);
  }

  return prop_duration;
}

static gchar *
gst_mpdparser_get_xml_node_content (xmlNode * a_node)
{
  xmlChar *content = NULL;

  content = xmlNodeGetContent (a_node);
  if (content) {
    GST_LOG (" - %s: %s", a_node->name, content);
  }

  return (gchar *) content;
}

static gchar *
gst_mpdparser_get_xml_node_namespace (xmlNode * a_node, const gchar * prefix)
{
  xmlNs *curr_ns;
  gchar *namespace = NULL;

  if (prefix == NULL) {
    /* return the default namespace */
    namespace = g_strdup ((gchar *) a_node->ns->href);
    if (namespace) {
      GST_LOG (" - default namespace: %s", namespace);
    }
  } else {
    /* look for the specified prefix in the namespace list */
    for (curr_ns = a_node->ns; curr_ns; curr_ns = curr_ns->next) {
      if (xmlStrcmp (curr_ns->prefix, (xmlChar *) prefix) == 0) {
        namespace = g_strdup ((gchar *) curr_ns->href);
        if (namespace) {
          GST_LOG (" - %s namespace: %s", curr_ns->prefix,
              curr_ns->href);
        }
      }
    }
  }

  return namespace;
}

static void
gst_mpdparser_parse_baseURL_node (GList ** list, xmlNode * a_node)
{
  GstBaseURL *new_base_url;

  new_base_url = g_slice_new0 (GstBaseURL);
  if (new_base_url == NULL) {
    GST_WARNING ("Allocation of BaseURL node failed!");
    return;
  }
  *list = g_list_append (*list, new_base_url);

  GST_LOG ("content of BaseURL node:");
  new_base_url->baseURL = gst_mpdparser_get_xml_node_content (a_node);

  GST_LOG ("attributes of BaseURL node:");
  new_base_url->serviceLocation =
      gst_mpdparser_get_xml_prop_string (a_node, "serviceLocation");
  new_base_url->byteRange =
      gst_mpdparser_get_xml_prop_string (a_node, "byteRange");
}

static void
gst_mpdparser_parse_descriptor_type_node (GList ** list, xmlNode * a_node)
{
  GstDescriptorType *new_descriptor;

  new_descriptor = g_slice_new0 (GstDescriptorType);
  if (new_descriptor == NULL) {
    GST_WARNING ("Allocation of DescriptorType node failed!");
    return;
  }
  *list = g_list_append (*list, new_descriptor);

  GST_LOG ("attributes of %s node:", a_node->name);
  new_descriptor->schemeIdUri =
      gst_mpdparser_get_xml_prop_string (a_node, "schemeIdUri");
  new_descriptor->value =
      gst_mpdparser_get_xml_prop_string (a_node, "value");
}

static void
gst_mpdparser_parse_content_component_node (GList ** list, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstContentComponentNode *new_content_component;

  new_content_component = g_slice_new0 (GstContentComponentNode);
  if (new_content_component == NULL) {
    GST_WARNING ("Allocation of ContentComponent node failed!");
    return;
  }
  *list = g_list_append (*list, new_content_component);

  GST_LOG ("attributes of ContentComponent node:");
  new_content_component->id =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "id", 0);
  new_content_component->lang =
      gst_mpdparser_get_xml_prop_string (a_node, "lang");
  new_content_component->contentType =
      gst_mpdparser_get_xml_prop_string (a_node, "contentType");
  new_content_component->par = gst_mpdparser_get_xml_prop_ratio (a_node, "par");

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Accessibility") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&new_content_component->Accessibility, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Role") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&new_content_component->Role, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Rating") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&new_content_component->Rating, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Viewpoint") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&new_content_component->Viewpoint, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_location_node (GList ** list, xmlNode * a_node)
{
  gchar *location;

  GST_LOG ("content of Location node:");
  location = gst_mpdparser_get_xml_node_content (a_node);

  *list = g_list_append (*list, location);
}

static void
gst_mpdparser_parse_subrepresentation_node (GList ** list, xmlNode * a_node)
{
  GstSubRepresentationNode *new_subrep;

  new_subrep = g_slice_new0 (GstSubRepresentationNode);
  if (new_subrep == NULL) {
    GST_WARNING ("Allocation of SubRepresentation node failed!");
    return;
  }
  *list = g_list_append (*list, new_subrep);

  GST_LOG ("attributes of SubRepresentation node:");
  new_subrep->level =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "level", 0);
  new_subrep->dependencyLevel =
      gst_mpdparser_get_xml_prop_uint_vector_type (a_node, "dependencyLevel", &new_subrep->size);
  new_subrep->bandwidth =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "bandwidth", 0);
  new_subrep->contentComponent =
      gst_mpdparser_get_xml_prop_string_vector_type (a_node, "contentComponent");

  /* RepresentationBase extension */
  gst_mpdparser_parse_representation_base_type (&new_subrep->RepresentationBase, a_node);
}

static void
gst_mpdparser_parse_segment_url_node (GList ** list, xmlNode * a_node)
{
  GstSegmentURLNode *new_segment_url;

  new_segment_url = g_slice_new0 (GstSegmentURLNode);
  if (new_segment_url == NULL) {
    GST_WARNING ("Allocation of SegmentURL node failed!");
    return;
  }
  *list = g_list_append (*list, new_segment_url);

  GST_LOG ("attributes of SegmentURL node:");
  new_segment_url->media =
      gst_mpdparser_get_xml_prop_string (a_node, "media");
  new_segment_url->mediaRange =
      gst_mpdparser_get_xml_prop_range (a_node, "mediaRange");
  new_segment_url->index =
      gst_mpdparser_get_xml_prop_string (a_node, "index");
  new_segment_url->indexRange =
      gst_mpdparser_get_xml_prop_range (a_node, "indexRange");
}

static void
gst_mpdparser_parse_url_type_node (GstURLType ** pointer, xmlNode * a_node)
{
  GstURLType *new_url_type;

  gst_mpdparser_free_url_type_node (*pointer);
  *pointer = new_url_type = g_slice_new0 (GstURLType);
  if (new_url_type == NULL) {
    GST_WARNING ("Allocation of URLType node failed!");
    return;
  }

  GST_LOG ("attributes of URLType node:");
  new_url_type->sourceURL =
      gst_mpdparser_get_xml_prop_string (a_node, "sourceURL");
  new_url_type->range =
      gst_mpdparser_get_xml_prop_range (a_node, "range");
}

static void
gst_mpdparser_parse_seg_base_type_ext (GstSegmentBaseType ** pointer, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstSegmentBaseType *seg_base_type;

  gst_mpdparser_free_seg_base_type_ext (*pointer);
  *pointer = seg_base_type = g_slice_new0 (GstSegmentBaseType);
  if (seg_base_type == NULL) {
    GST_WARNING ("Allocation of SegmentBaseType node failed!");
    return;
  }

  GST_LOG ("attributes of SegmentBaseType extension:");
  seg_base_type->timescale =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "timescale",
      0);
  seg_base_type->presentationTimeOffset =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node,
      "presentationTimeOffset", 0);
  seg_base_type->indexRange =
      gst_mpdparser_get_xml_prop_string (a_node, "indexRange");
  seg_base_type->indexRangeExact =
      gst_mpdparser_get_xml_prop_boolean (a_node, "indexRangeExact");

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Initialization") == 0 ||
          xmlStrcmp (cur_node->name, (xmlChar *) "Initialisation") == 0) {
        gst_mpdparser_parse_url_type_node (&seg_base_type->Initialization, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "RepresentationIndex") == 0) {
        gst_mpdparser_parse_url_type_node (&seg_base_type->RepresentationIndex, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_s_node (GList ** list, xmlNode * a_node)
{
  GstSNode *new_s_node;

  new_s_node = g_slice_new0 (GstSNode);
  if (new_s_node == NULL) {
    GST_WARNING ("Allocation of S node failed!");
    return;
  }
  *list = g_list_append (*list, new_s_node);

  GST_LOG ("attributes of S node:");
  new_s_node->t =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "t", 0);
  new_s_node->d =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "d", 0);
  new_s_node->r =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "r", 0);
}

static void
gst_mpdparser_parse_segment_timeline_node (GstSegmentTimelineNode ** pointer, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstSegmentTimelineNode *new_seg_timeline;

  gst_mpdparser_free_segment_timeline_node (*pointer);
  *pointer = new_seg_timeline = g_slice_new0 (GstSegmentTimelineNode);
  if (new_seg_timeline == NULL) {
    GST_WARNING ("Allocation of SegmentTimeline node failed!");
    return;
  }

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "S") == 0) {
        gst_mpdparser_parse_s_node (&new_seg_timeline->S, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_mult_seg_base_type_ext (GstMultSegmentBaseType ** pointer, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstMultSegmentBaseType *mult_seg_base_type;

  gst_mpdparser_free_mult_seg_base_type_ext (*pointer);
  *pointer = mult_seg_base_type = g_slice_new0 (GstMultSegmentBaseType);
  if (mult_seg_base_type == NULL) {
    GST_WARNING ("Allocation of MultipleSegmentBaseType node failed!");
    return;
  }

  GST_LOG ("attributes of MultipleSegmentBaseType extension:");
  mult_seg_base_type->duration =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "duration", 0);
  mult_seg_base_type->startNumber =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "startNumber", 1);

  GST_LOG ("extension of MultipleSegmentBaseType extension:");
  gst_mpdparser_parse_seg_base_type_ext (&mult_seg_base_type->SegBaseType, a_node);

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentTimeline") == 0) {
        gst_mpdparser_parse_segment_timeline_node (&mult_seg_base_type->SegmentTimeline, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "BitstreamSwitching") == 0) {
        gst_mpdparser_parse_url_type_node (&mult_seg_base_type->BitstreamSwitching, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_segment_list_node (GList ** list, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstSegmentListNode *new_segment_list;

  new_segment_list = g_slice_new0 (GstSegmentListNode);
  if (new_segment_list == NULL) {
    GST_WARNING ("Allocation of SegmentList node failed!");
    return;
  }
  *list = g_list_append (*list, new_segment_list);

  GST_LOG ("extension of SegmentList node:");
  gst_mpdparser_parse_mult_seg_base_type_ext (&new_segment_list->MultSegBaseType, a_node);

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentURL") == 0) {
        gst_mpdparser_parse_segment_url_node (&new_segment_list->SegmentURL, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_representation_base_type (GstRepresentationBaseType ** pointer, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstRepresentationBaseType *representation_base;

  gst_mpdparser_free_representation_base_type (*pointer);
  *pointer = representation_base = g_slice_new0 (GstRepresentationBaseType);
  if (representation_base == NULL) {
    GST_WARNING ("Allocation of RepresentationBaseType node failed!");
    return;
  }

  GST_LOG ("attributes of RepresentationBaseType extension:");
  representation_base->profiles =
      gst_mpdparser_get_xml_prop_string (a_node, "profiles");
  representation_base->width =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "width", 0);
  representation_base->height =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "height",
      0);
  representation_base->sar =
      gst_mpdparser_get_xml_prop_ratio (a_node, "sar");
  representation_base->frameRate =
      gst_mpdparser_get_xml_prop_framerate (a_node, "frameRate");
  representation_base->audioSamplingRate =
      gst_mpdparser_get_xml_prop_string (a_node, "audioSamplingRate");
  representation_base->mimeType =
      gst_mpdparser_get_xml_prop_string (a_node, "mimeType");
  representation_base->segmentProfiles =
      gst_mpdparser_get_xml_prop_string (a_node, "segmentProfiles");
  representation_base->codecs =
      gst_mpdparser_get_xml_prop_string (a_node, "codecs");
  representation_base->maximumSAPPeriod =
      gst_mpdparser_get_xml_prop_double (a_node, "maximumSAPPeriod");
  representation_base->startWithSAP =
      gst_mpdparser_get_xml_prop_SAP_type (a_node, "startWithSAP");
  representation_base->maxPlayoutRate =
      gst_mpdparser_get_xml_prop_double (a_node, "maxPlayoutRate");
  representation_base->codingDependency =
      gst_mpdparser_get_xml_prop_boolean (a_node, "codingDependency");
  representation_base->scanType =
      gst_mpdparser_get_xml_prop_string (a_node, "scanType");

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "FramePacking") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&representation_base->FramePacking, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "AudioChannelConfiguration") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&representation_base->AudioChannelConfiguration, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "ContentProtection") == 0) {
        gst_mpdparser_parse_descriptor_type_node (&representation_base->ContentProtection, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_representation_node (GList ** list, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstRepresentationNode *new_representation;

  new_representation = g_slice_new0 (GstRepresentationNode);
  if (new_representation == NULL) {
    GST_WARNING ("Allocation of Representation node failed!");
    return;
  }
  *list = g_list_append (*list, new_representation);

  GST_LOG ("attributes of Representation node:");
  new_representation->id =
      gst_mpdparser_get_xml_prop_string (a_node, "id");
  new_representation->bandwidth =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "bandwidth",
      0);
  new_representation->qualityRanking =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node,
      "qualityRanking", 0);
  new_representation->dependencyId =
      gst_mpdparser_get_xml_prop_string_vector_type (a_node, "dependencyId");
  new_representation->mediaStreamStructureId =
      gst_mpdparser_get_xml_prop_string_vector_type (a_node,
      "mediaStreamStructureId");

  /* RepresentationBase extension */
  gst_mpdparser_parse_representation_base_type (&new_representation->RepresentationBase, a_node);

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentBase") == 0) {
        gst_mpdparser_parse_seg_base_type_ext (&new_representation->SegmentBase, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentTemplate") == 0) {
        gst_mpdparser_parse_segment_template_node (&new_representation->SegmentTemplate, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentList") == 0) {
        gst_mpdparser_parse_segment_list_node (&new_representation->SegmentList, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "BaseURL") == 0) {
        gst_mpdparser_parse_baseURL_node (&new_representation->BaseURLs, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "SubRepresentation") == 0) {
        gst_mpdparser_parse_subrepresentation_node (&new_representation->SubRepresentations, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_adaptation_set_node (GList ** list, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstAdaptationSetNode *new_adap_set;

  new_adap_set = g_slice_new0 (GstAdaptationSetNode);
  if (new_adap_set == NULL) {
    GST_WARNING ("Allocation of AdaptationSet node failed!");
    return;
  }
  *list = g_list_append (*list, new_adap_set);

  GST_LOG ("attributes of AdaptationSet node:");
  new_adap_set->id =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "id", 0);
  new_adap_set->group =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "group", 0);
  new_adap_set->lang =
      gst_mpdparser_get_xml_prop_string (a_node, "lang");
  new_adap_set->contentType =
      gst_mpdparser_get_xml_prop_string (a_node, "contentType");
  new_adap_set->par = gst_mpdparser_get_xml_prop_ratio (a_node, "par");
  new_adap_set->minBandwidth =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node,
      "minBandwidth", 0);
  new_adap_set->maxBandwidth =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node,
      "maxBandwidth", 0);
  new_adap_set->minWidth =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "minWidth",
      0);
  new_adap_set->maxWidth =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "maxWidth",
      0);
  new_adap_set->minHeight =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "minHeight",
      0);
  new_adap_set->maxHeight =
      gst_mpdparser_get_xml_prop_unsigned_integer (a_node, "maxHeight",
      0);
  new_adap_set->minFrameRate =
      gst_mpdparser_get_xml_prop_framerate (a_node, "minFrameRate");
  new_adap_set->maxFrameRate =
      gst_mpdparser_get_xml_prop_framerate (a_node, "maxFrameRate");
  new_adap_set->segmentAlignment =
      gst_mpdparser_get_xml_prop_cond_uint (a_node, "segmentAlignment");
  new_adap_set->subsegmentAlignment =
      gst_mpdparser_get_xml_prop_cond_uint (a_node, "subsegmentAlignment");
  new_adap_set->subsegmentStartsWithSAP =
      gst_mpdparser_get_xml_prop_SAP_type (a_node, "subsegmentStartsWithSAP");
  new_adap_set->bitstreamSwitching =
      gst_mpdparser_get_xml_prop_boolean (a_node, "bitstreamSwitching");

  /* RepresentationBase extension */
  gst_mpdparser_parse_representation_base_type (&new_adap_set->RepresentationBase, a_node);

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Representation") == 0) {
        gst_mpdparser_parse_representation_node (&new_adap_set->Representations, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "BaseURL") == 0) {
        gst_mpdparser_parse_baseURL_node (&new_adap_set->BaseURLs, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentBase") == 0) {
        gst_mpdparser_parse_seg_base_type_ext (&new_adap_set->SegmentBase, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "ContentComponent") == 0) {
        gst_mpdparser_parse_content_component_node (&new_adap_set->ContentComponents, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentTemplate") == 0) {
        gst_mpdparser_parse_segment_template_node (&new_adap_set->SegmentTemplate, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_subset_node (GList ** list, xmlNode * a_node)
{
  GstSubsetNode *new_subset;

  new_subset = g_slice_new0 (GstSubsetNode);
  if (new_subset == NULL) {
    GST_WARNING ("Allocation of Subset node failed!");
    return;
  }
  *list = g_list_append (*list, new_subset);

  GST_LOG ("attributes of Subset node:");
  new_subset->contains =
      gst_mpdparser_get_xml_prop_uint_vector_type (a_node, "contains", &new_subset->size);
}

static void
gst_mpdparser_parse_segment_template_node (GstSegmentTemplateNode ** pointer, xmlNode * a_node)
{
  GstSegmentTemplateNode *new_segment_template;

  gst_mpdparser_free_segment_template_node (*pointer);
  *pointer = new_segment_template = g_slice_new0 (GstSegmentTemplateNode);
  if (new_segment_template == NULL) {
    GST_WARNING ("Allocation of SegmentTemplate node failed!");
    return;
  }

  GST_LOG ("extension of SegmentTemplate node:");
  gst_mpdparser_parse_mult_seg_base_type_ext (&new_segment_template->MultSegBaseType, a_node);

  GST_LOG ("attributes of SegmentTemplate node:");
  new_segment_template->media = gst_mpdparser_get_xml_prop_string (a_node, "media");
  new_segment_template->index = gst_mpdparser_get_xml_prop_string (a_node, "index");
  new_segment_template->initialization = gst_mpdparser_get_xml_prop_string (a_node, "initialization");
  new_segment_template->bitstreamSwitching = gst_mpdparser_get_xml_prop_string (a_node, "bitstreamSwitching");
}

static void
gst_mpdparser_parse_period_node (GList ** list, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstPeriodNode *new_period;

  new_period = g_slice_new0 (GstPeriodNode);
  if (new_period == NULL) {
    GST_WARNING ("Allocation of Period node failed!");
    return;
  }
  *list = g_list_append (*list, new_period);

  GST_LOG ("attributes of Period node:");
  new_period->id = gst_mpdparser_get_xml_prop_string (a_node, "id");
  new_period->start =
      gst_mpdparser_get_xml_prop_duration (a_node, "start");
  new_period->duration =
      gst_mpdparser_get_xml_prop_duration (a_node, "duration");
  new_period->bitstreamSwitching =
      gst_mpdparser_get_xml_prop_boolean (a_node, "bitstreamSwitching");

  /* explore children nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Representation") == 0) {
        gst_mpdparser_parse_representation_node (&new_period->Representations, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "AdaptationSet") == 0) {
        gst_mpdparser_parse_adaptation_set_node (&new_period->AdaptationSets, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "SegmentBase") == 0) {
        gst_mpdparser_parse_seg_base_type_ext (&new_period->SegmentBase, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "SegmentTemplate") == 0) {
        gst_mpdparser_parse_segment_template_node (&new_period->SegmentTemplate, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Subset") == 0) {
        gst_mpdparser_parse_subset_node (&new_period->Subsets, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "BaseURL") == 0) {
        gst_mpdparser_parse_baseURL_node (&new_period->BaseURLs, cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_program_info_node (GstProgramInformationNode ** pointer, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstProgramInformationNode *new_prog_info;

  gst_mpdparser_free_prog_info_node (*pointer);
  *pointer = new_prog_info = g_slice_new0 (GstProgramInformationNode);
  if (new_prog_info == NULL) {
    GST_WARNING ("Allocation of ProgramInfo node failed!");
    return;
  }

  GST_LOG ("attributes of ProgramInformation node:");
  new_prog_info->moreInformationURL =
      gst_mpdparser_get_xml_prop_string (a_node, "moreInformationURL");

  /* explore children nodes */
  GST_LOG ("children of ProgramInformation node:");
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Title") == 0) {
        new_prog_info->Title =
            gst_mpdparser_get_xml_node_content (cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Source") == 0) {
        new_prog_info->Source =
            gst_mpdparser_get_xml_node_content (cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Copyright") == 0) {
        new_prog_info->Copyright =
            gst_mpdparser_get_xml_node_content (cur_node);
      }
    }
  }
}

static void
gst_mpdparser_parse_metrics_range_node (GList ** list, xmlNode * a_node)
{
  GstMetricsRangeNode *new_metrics_range;

  new_metrics_range = g_slice_new0 (GstMetricsRangeNode);
  if (new_metrics_range == NULL) {
    GST_WARNING ("Allocation of Metrics Range node failed!");
    return;
  }
  *list = g_list_append (*list, new_metrics_range);

  GST_LOG ("attributes of Metrics Range node:");
  new_metrics_range->starttime =
      gst_mpdparser_get_xml_prop_duration (a_node, "starttime");
  new_metrics_range->duration =
      gst_mpdparser_get_xml_prop_duration (a_node, "duration");
}

static void
gst_mpdparser_parse_metrics_node (GList ** list, xmlNode * a_node)
{
  xmlNode *cur_node;
  GstMetricsNode *new_metrics;

  new_metrics = g_slice_new0 (GstMetricsNode);
  if (new_metrics == NULL) {
    GST_WARNING ("Allocation of Metrics node failed!");
    return;
  }
  *list = g_list_append (*list, new_metrics);

  GST_LOG ("attributes of Metrics node:");
  new_metrics->metrics =
      gst_mpdparser_get_xml_prop_string (a_node, "metrics");

  /* explore children nodes */
  GST_LOG ("children of Metrics node:");
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Range") == 0) {
        gst_mpdparser_parse_metrics_range_node (&new_metrics->MetricsRanges, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Reporting") == 0) {
        /* No reporting scheme is specified in this part of ISO/IEC 23009.
         * It is expected that external specifications may define formats
         * and delivery for the reporting data. */
        GST_LOG (" - Reporting node found (unknown structure)");
      }
    }
  }
}

static void
gst_mpdparser_parse_root_node (GstMpdClient * client, xmlNode *a_node)
{
  xmlNode *cur_node;
  GstMPDNode *new_mpd;

  gst_mpdparser_free_mpd_node (client->mpd_node);
  client->mpd_node = new_mpd = g_slice_new0 (GstMPDNode);
  if (new_mpd == NULL) {
    GST_WARNING ("Allocation of MPD node failed!");
    return;
  }

  GST_LOG ("namespaces of root MPD node:");
  new_mpd->default_namespace =
      gst_mpdparser_get_xml_node_namespace (a_node, NULL);
  new_mpd->namespace_xsi =
      gst_mpdparser_get_xml_node_namespace (a_node, "xsi");
  new_mpd->namespace_ext =
      gst_mpdparser_get_xml_node_namespace (a_node, "ext");

  GST_LOG ("attributes of root MPD node:");
  new_mpd->schemaLocation =
      gst_mpdparser_get_xml_prop_string (a_node, "schemaLocation");
  new_mpd->id = gst_mpdparser_get_xml_prop_string (a_node, "id");
  new_mpd->profiles =
      gst_mpdparser_get_xml_prop_string (a_node, "profiles");
  new_mpd->type = gst_mpdparser_get_xml_prop_type (a_node, "type");
  new_mpd->availabilityStartTime =
      gst_mpdparser_get_xml_prop_dateTime (a_node, "availabilityStartTime");
  new_mpd->availabilityEndTime =
      gst_mpdparser_get_xml_prop_dateTime (a_node, "availabilityEndTime");
  new_mpd->mediaPresentationDuration =
      gst_mpdparser_get_xml_prop_duration (a_node,
      "mediaPresentationDuration");
  new_mpd->minimumUpdatePeriod =
      gst_mpdparser_get_xml_prop_duration (a_node,
      "minimumUpdatePeriod");
  new_mpd->minBufferTime =
      gst_mpdparser_get_xml_prop_duration (a_node, "minBufferTime");
  new_mpd->timeShiftBufferDepth =
      gst_mpdparser_get_xml_prop_duration (a_node,
      "timeShiftBufferDepth");
  new_mpd->suggestedPresentationDelay =
      gst_mpdparser_get_xml_prop_duration (a_node,
      "suggestedPresentationDelay");
  new_mpd->maxSegmentDuration =
      gst_mpdparser_get_xml_prop_duration (a_node,
      "maxSegmentDuration");
  new_mpd->maxSubsegmentDuration =
      gst_mpdparser_get_xml_prop_duration (a_node,
      "maxSubsegmentDuration");

  /* explore children Period nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Period") == 0) {
        gst_mpdparser_parse_period_node (&new_mpd->Periods, cur_node);
      } else if (xmlStrcmp (cur_node->name,
              (xmlChar *) "ProgramInformation") == 0) {
        gst_mpdparser_parse_program_info_node (&new_mpd->ProgramInfo, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "BaseURL") == 0) {
        gst_mpdparser_parse_baseURL_node (&new_mpd->BaseURLs, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Location") == 0) {
        gst_mpdparser_parse_location_node (&new_mpd->Locations, cur_node);
      } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Metrics") == 0) {
        gst_mpdparser_parse_metrics_node (&new_mpd->Metrics, cur_node);
      }
    }
  }
}

/* comparison functions */
static int
strncmp_ext (const char *s1, const char *s2)
{
  if (s1 == NULL && s2 == NULL)
    return 0;
  if (s1 == NULL && s2 != NULL)
    return 1;
  if (s2 == NULL && s1 != NULL)
    return 1;
  return strncmp (s1, s2, strlen (s2));
}

/* navigation functions */
static GstPeriodNode *
gst_mpdparser_get_next_period (GList * Periods, GstPeriodNode * prev_period)
{
  GList *list = NULL;

  if (Periods == NULL)
    return NULL;

  if (prev_period == NULL) {
    /* return the first period in the list */
    list = g_list_first (Periods);
  } else {
    /* found prev_period in the list */
    list = g_list_find (Periods, prev_period);
    /* next period */
    list = g_list_next (list);
  }

  return list ? (GstPeriodNode *) list->data : NULL;
}

static GstAdaptationSetNode *
gst_mpdparser_get_first_adapt_set (GList * AdaptationSets)
{
  GList *list = NULL;

  if (AdaptationSets == NULL)
    return NULL;

  list = g_list_first (AdaptationSets);

  return list ? (GstAdaptationSetNode *) list->data : NULL;
}

static GstAdaptationSetNode *
gst_mpdparser_get_first_adapt_set_with_mimeType (GList * AdaptationSets,
    const gchar * mimeType)
{
  GList *list;
  GstAdaptationSetNode *adapt_set;

  if (AdaptationSets == NULL)
    return NULL;

  for (list = g_list_first (AdaptationSets); list; list = g_list_next (list)) {
    adapt_set = (GstAdaptationSetNode *) list->data;
    if (adapt_set) {
      gchar *this_mimeType = NULL;
      GstRepresentationNode *rep;
      rep =
          gst_mpdparser_get_lowest_representation (adapt_set->Representations);
      if (rep->RepresentationBase)
        this_mimeType = rep->RepresentationBase->mimeType;
      if (!this_mimeType && adapt_set->RepresentationBase) {
        this_mimeType = adapt_set->RepresentationBase->mimeType;
      }
      if (strncmp_ext (this_mimeType, mimeType) == 0)
        return adapt_set;
    }
  }

  return NULL;
}

/* if idx < 0, returns the highest adaptation set with the given mimeType
 * if idx >= 0, returns the highest adaptation set with the given mimeType and an index <= idx
 */
static GstAdaptationSetNode *
gst_mpdparser_get_adapt_set_with_mimeType_and_idx (GList * AdaptationSets,
    const gchar * mimeType, gint idx)
{
  GList *list;
  GstAdaptationSetNode *adapt_set, *selected = NULL;
  gint i = 0;

  if (AdaptationSets == NULL)
    return NULL;

  for (list = g_list_first (AdaptationSets); list; list = g_list_next (list)) {
    adapt_set = (GstAdaptationSetNode *) list->data;
    if (adapt_set) {
      gchar *this_mimeType = NULL;
      GstRepresentationNode *rep;
      rep =
          gst_mpdparser_get_lowest_representation (adapt_set->Representations);
      if (rep->RepresentationBase)
        this_mimeType = rep->RepresentationBase->mimeType;
      if (!this_mimeType && adapt_set->RepresentationBase) {
        this_mimeType = adapt_set->RepresentationBase->mimeType;
      }
      if (strncmp_ext (this_mimeType, mimeType) == 0) {
        if (idx < 0 || i <= idx)
          selected = adapt_set;
        i++;
      }
    }
  }

  return selected;
}

static GstAdaptationSetNode *
gst_mpdparser_get_first_adapt_set_with_mimeType_and_lang (GList *
    AdaptationSets, const gchar * mimeType, const gchar * lang)
{
  GList *list;
  GstAdaptationSetNode *adapt_set;

  if (AdaptationSets == NULL)
    return NULL;

  for (list = g_list_first (AdaptationSets); list; list = g_list_next (list)) {
    adapt_set = (GstAdaptationSetNode *) list->data;
    if (adapt_set) {
      GstRepresentationNode *rep;
      gchar *this_lang = adapt_set->lang;
      gchar *this_mimeType = NULL;
      rep =
          gst_mpdparser_get_lowest_representation (adapt_set->Representations);
      if (rep->RepresentationBase)
        this_mimeType = rep->RepresentationBase->mimeType;
      if (!this_mimeType && adapt_set->RepresentationBase) {
        this_mimeType = adapt_set->RepresentationBase->mimeType;
      }
      if (strncmp_ext (this_mimeType, mimeType) == 0
          && strncmp_ext (this_lang, lang) == 0)
        return adapt_set;
    }
  }

  return NULL;
}

static GstRepresentationNode *
gst_mpdparser_get_lowest_representation (GList * Representations)
{
  GList *list = NULL;

  if (Representations == NULL)
    return NULL;

  list = g_list_first (Representations);

  return list ? (GstRepresentationNode *) list->data : NULL;
}

#if 0
static GstRepresentationNode *
gst_mpdparser_get_highest_representation (GList * Representations)
{
  GList *list = NULL;

  if (Representations == NULL)
    return NULL;

  list = g_list_last (Representations);

  return list ? (GstRepresentationNode *) list->data : NULL;
}

static GstRepresentationNode *
gst_mpdparser_get_representation_with_max_bandwidth (GList * Representations,
    gint max_bandwidth)
{
  GList *list = NULL;
  GstRepresentationNode *representation, *best_rep = NULL;

  if (Representations == NULL)
    return NULL;

  if (max_bandwidth <= 0)       /* 0 => get highest representation available */
    return gst_mpdparser_get_highest_representation (Representations);

  for (list = g_list_first (Representations); list; list = g_list_next (list)) {
    representation = (GstRepresentationNode *) list->data;
    if (representation && representation->bandwidth <= max_bandwidth) {
      best_rep = representation;
    }
  }

  return best_rep;
}
#endif

static GstSegmentBaseType *
gst_mpdparser_get_segment_base (GstPeriodNode *Period,
    GstAdaptationSetNode * AdaptationSet, GstRepresentationNode * Representation)
{
  GstSegmentBaseType *SegmentBase = NULL;

  if (Representation && Representation->SegmentBase) {
    SegmentBase = Representation->SegmentBase;
  } else if (AdaptationSet) {
    SegmentBase = AdaptationSet->SegmentBase;
  } else {
    SegmentBase = Period->SegmentBase;
  }

  return SegmentBase;
}

gint
gst_mpdparser_get_rep_idx_with_max_bandwidth (GList * Representations,
    gint max_bandwidth)
{
  GList *list = NULL, *best = NULL;
  GstRepresentationNode *representation;

  if (Representations == NULL)
    return -1;

  if (max_bandwidth <= 0) /* 0 => get lowest representation available */
    return 0;

  for (list = g_list_first (Representations); list; list = g_list_next (list)) {
    representation = (GstRepresentationNode *) list->data;
    if (representation && representation->bandwidth <= max_bandwidth) {
      best = list;
    }
  }

  return best ? g_list_position (Representations, best) : -1;
}

static GstSegmentListNode *
gst_mpdparser_get_first_segment_list (GList * SegmentList)
{
  GList *list = NULL;

  if (SegmentList == NULL)
    return NULL;

  list = g_list_first (SegmentList);

  return list ? (GstSegmentListNode *) list->data : NULL;
}

/* memory management functions */
static void
gst_mpdparser_free_mpd_node (GstMPDNode * mpd_node)
{
  if (mpd_node) {
    g_free (mpd_node->default_namespace);
    g_free (mpd_node->namespace_xsi);
    g_free (mpd_node->namespace_ext);
    g_free (mpd_node->schemaLocation);
    g_free (mpd_node->id);
    g_free (mpd_node->profiles);
    if (mpd_node->availabilityStartTime)
      gst_date_time_unref (mpd_node->availabilityStartTime);
    if (mpd_node->availabilityEndTime)
      gst_date_time_unref (mpd_node->availabilityEndTime);
    gst_mpdparser_free_prog_info_node (mpd_node->ProgramInfo);
    g_list_foreach (mpd_node->BaseURLs,
        (GFunc) gst_mpdparser_free_base_url_node, NULL);
    g_list_free (mpd_node->BaseURLs);
    g_list_foreach (mpd_node->Locations, (GFunc) g_free, NULL);
    g_list_free (mpd_node->Locations);
    g_list_foreach (mpd_node->Periods, (GFunc) gst_mpdparser_free_period_node,
        NULL);
    g_list_free (mpd_node->Periods);
    g_list_foreach (mpd_node->Metrics, (GFunc) gst_mpdparser_free_metrics_node,
        NULL);
    g_list_free (mpd_node->Metrics);
    g_slice_free (GstMPDNode, mpd_node);
  }
}

static void
gst_mpdparser_free_prog_info_node (GstProgramInformationNode * prog_info_node)
{
  if (prog_info_node) {
    g_free (prog_info_node->moreInformationURL);
    g_free (prog_info_node->Title);
    g_free (prog_info_node->Source);
    g_free (prog_info_node->Copyright);
    g_slice_free (GstProgramInformationNode, prog_info_node);
  }
}

static void
gst_mpdparser_free_metrics_node (GstMetricsNode * metrics_node)
{
  if (metrics_node) {
    g_free (metrics_node->metrics);
    g_list_foreach (metrics_node->MetricsRanges,
        (GFunc) gst_mpdparser_free_metrics_range_node, NULL);
    g_list_free (metrics_node->MetricsRanges);
  }
}

static void
gst_mpdparser_free_metrics_range_node (GstMetricsRangeNode * metrics_range_node)
{
  if (metrics_range_node) {
    g_slice_free (GstMetricsRangeNode, metrics_range_node);
  }
}

static void
gst_mpdparser_free_period_node (GstPeriodNode * period_node)
{
  if (period_node) {
    g_free (period_node->id);
    gst_mpdparser_free_seg_base_type_ext (period_node->SegmentBase);
    gst_mpdparser_free_segment_template_node (period_node->SegmentTemplate);
    g_list_foreach (period_node->AdaptationSets,
        (GFunc) gst_mpdparser_free_adaptation_set_node, NULL);
    g_list_free (period_node->AdaptationSets);
    g_list_foreach (period_node->Representations,
        (GFunc) gst_mpdparser_free_representation_node, NULL);
    g_list_free (period_node->Representations);
    g_list_foreach (period_node->Subsets,
        (GFunc) gst_mpdparser_free_subset_node, NULL);
    g_list_free (period_node->Subsets);
    g_list_foreach (period_node->BaseURLs,
        (GFunc) gst_mpdparser_free_base_url_node, NULL);
    g_list_free (period_node->BaseURLs);
    g_slice_free (GstPeriodNode, period_node);
  }
}

static void
gst_mpdparser_free_subset_node (GstSubsetNode * subset_node)
{
  if (subset_node) {
    g_free (subset_node->contains);
    g_slice_free (GstSubsetNode, subset_node);
  }
}

static void
gst_mpdparser_free_segment_template_node (GstSegmentTemplateNode * segment_template_node)
{
  if (segment_template_node) {
    g_free (segment_template_node->media);
    g_free (segment_template_node->index);
    g_free (segment_template_node->initialization);
    g_free (segment_template_node->bitstreamSwitching);
    /* MultipleSegmentBaseType extension */
    gst_mpdparser_free_mult_seg_base_type_ext
        (segment_template_node->MultSegBaseType);
    g_slice_free (GstSegmentTemplateNode, segment_template_node);
  }
}

static void
gst_mpdparser_free_representation_base_type (GstRepresentationBaseType *
    representation_base)
{
  if (representation_base) {
    g_free (representation_base->profiles);
    g_slice_free (GstRatio, representation_base->sar);
    g_slice_free (GstFrameRate, representation_base->frameRate);
    g_free (representation_base->audioSamplingRate);
    g_free (representation_base->mimeType);
    g_free (representation_base->segmentProfiles);
    g_free (representation_base->codecs);
    g_free (representation_base->scanType);
    g_list_foreach (representation_base->FramePacking,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (representation_base->FramePacking);
    g_list_foreach (representation_base->AudioChannelConfiguration,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (representation_base->AudioChannelConfiguration);
    g_list_foreach (representation_base->ContentProtection,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (representation_base->ContentProtection);
    g_slice_free (GstRepresentationBaseType, representation_base);
  }
}

static void
gst_mpdparser_free_adaptation_set_node (GstAdaptationSetNode *
    adaptation_set_node)
{
  if (adaptation_set_node) {
    g_free (adaptation_set_node->lang);
    g_free (adaptation_set_node->contentType);
    g_slice_free (GstRatio, adaptation_set_node->par);
    g_slice_free (GstFrameRate, adaptation_set_node->minFrameRate);
    g_slice_free (GstFrameRate, adaptation_set_node->maxFrameRate);
    g_slice_free (GstConditionalUintType,
        adaptation_set_node->segmentAlignment);
    g_slice_free (GstConditionalUintType,
        adaptation_set_node->subsegmentAlignment);
    gst_mpdparser_free_representation_base_type
        (adaptation_set_node->RepresentationBase);
    gst_mpdparser_free_seg_base_type_ext (adaptation_set_node->SegmentBase);
    gst_mpdparser_free_segment_template_node (adaptation_set_node->SegmentTemplate);
    g_list_foreach (adaptation_set_node->BaseURLs,
        (GFunc) gst_mpdparser_free_base_url_node, NULL);
    g_list_free (adaptation_set_node->BaseURLs);
    g_list_foreach (adaptation_set_node->Representations,
        (GFunc) gst_mpdparser_free_representation_node, NULL);
    g_list_free (adaptation_set_node->Representations);
    g_list_foreach (adaptation_set_node->ContentComponents,
        (GFunc) gst_mpdparser_free_content_component_node, NULL);
    g_list_free (adaptation_set_node->ContentComponents);
    g_slice_free (GstAdaptationSetNode, adaptation_set_node);
  }
}

static void
gst_mpdparser_free_representation_node (GstRepresentationNode *
    representation_node)
{
  if (representation_node) {
    g_free (representation_node->id);
    g_strfreev (representation_node->dependencyId);
    g_strfreev (representation_node->mediaStreamStructureId);
    gst_mpdparser_free_representation_base_type
        (representation_node->RepresentationBase);
    g_list_foreach (representation_node->SubRepresentations,
        (GFunc) gst_mpdparser_free_subrepresentation_node, NULL);
    g_list_free (representation_node->SubRepresentations);
    gst_mpdparser_free_seg_base_type_ext (representation_node->SegmentBase);
    gst_mpdparser_free_segment_template_node (representation_node->SegmentTemplate);
    g_list_foreach (representation_node->SegmentList,
        (GFunc) gst_mpdparser_free_segment_list_node, NULL);
    g_list_free (representation_node->SegmentList);
    g_list_foreach (representation_node->BaseURLs,
        (GFunc) gst_mpdparser_free_base_url_node, NULL);
    g_list_free (representation_node->BaseURLs);
    g_slice_free (GstRepresentationNode, representation_node);
  }
}

static void
gst_mpdparser_free_subrepresentation_node (GstSubRepresentationNode * subrep_node)
{
  if (subrep_node) {
    gst_mpdparser_free_representation_base_type (subrep_node->RepresentationBase);
    g_free (subrep_node->dependencyLevel);
    g_strfreev (subrep_node->contentComponent);
  }
}

static void
gst_mpdparser_free_s_node (GstSNode * s_node)
{
  if (s_node) {
    g_slice_free (GstSNode, s_node);
  }
}

static void
gst_mpdparser_free_segment_timeline_node (GstSegmentTimelineNode * seg_timeline)
{
  if (seg_timeline) {
    g_list_foreach (seg_timeline->S, (GFunc) gst_mpdparser_free_s_node, NULL);
    g_list_free (seg_timeline->S);
    g_slice_free (GstSegmentTimelineNode, seg_timeline);
  }
}

static void
gst_mpdparser_free_url_type_node (GstURLType * url_type_node)
{
  if (url_type_node) {
    g_free (url_type_node->sourceURL);
    g_slice_free (GstRange, url_type_node->range);
    g_slice_free (GstURLType, url_type_node);
  }
}

static void
gst_mpdparser_free_seg_base_type_ext (GstSegmentBaseType * seg_base_type)
{
  if (seg_base_type) {
    g_free (seg_base_type->indexRange);
    gst_mpdparser_free_url_type_node (seg_base_type->Initialization);
    gst_mpdparser_free_url_type_node (seg_base_type->RepresentationIndex);
    g_slice_free (GstSegmentBaseType, seg_base_type);
  }
}

static void
gst_mpdparser_free_mult_seg_base_type_ext (GstMultSegmentBaseType *
    mult_seg_base_type)
{
  if (mult_seg_base_type) {
    /* SegmentBaseType extension */
    gst_mpdparser_free_seg_base_type_ext (mult_seg_base_type->SegBaseType);
    gst_mpdparser_free_segment_timeline_node
        (mult_seg_base_type->SegmentTimeline);
    gst_mpdparser_free_url_type_node (mult_seg_base_type->BitstreamSwitching);
    g_slice_free (GstMultSegmentBaseType, mult_seg_base_type);
  }
}

static void
gst_mpdparser_free_segment_list_node (GstSegmentListNode * segment_list_node)
{
  if (segment_list_node) {
    g_list_foreach (segment_list_node->SegmentURL,
        (GFunc) gst_mpdparser_free_segment_url_node, NULL);
    g_list_free (segment_list_node->SegmentURL);
    /* MultipleSegmentBaseType extension */
    gst_mpdparser_free_mult_seg_base_type_ext
        (segment_list_node->MultSegBaseType);
    g_slice_free (GstSegmentListNode, segment_list_node);
  }
}

static void
gst_mpdparser_free_segment_url_node (GstSegmentURLNode * segment_url)
{
  if (segment_url) {
    g_free (segment_url->media);
    g_slice_free (GstRange, segment_url->mediaRange);
    g_free (segment_url->index);
    g_slice_free (GstRange, segment_url->indexRange);
    g_slice_free (GstSegmentURLNode, segment_url);
  }
}

static void
gst_mpdparser_free_base_url_node (GstBaseURL * base_url_node)
{
  if (base_url_node) {
    g_free (base_url_node->baseURL);
    g_free (base_url_node->serviceLocation);
    g_free (base_url_node->byteRange);
    g_slice_free (GstBaseURL, base_url_node);
  }
}

static void
gst_mpdparser_free_descriptor_type_node (GstDescriptorType * descriptor_type)
{
  if (descriptor_type) {
    g_free (descriptor_type->schemeIdUri);
    g_free (descriptor_type->value);
  }
}

static void
gst_mpdparser_free_content_component_node (GstContentComponentNode * content_component_node)
{
  if (content_component_node) {
    g_free (content_component_node->lang);
    g_free (content_component_node->contentType);
    g_slice_free (GstRatio, content_component_node->par);
    g_list_foreach (content_component_node->Accessibility,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (content_component_node->Accessibility);
    g_list_foreach (content_component_node->Role,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (content_component_node->Role);
    g_list_foreach (content_component_node->Rating,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (content_component_node->Rating);
    g_list_foreach (content_component_node->Viewpoint,
        (GFunc) gst_mpdparser_free_descriptor_type_node, NULL);
    g_list_free (content_component_node->Viewpoint);
    g_slice_free (GstContentComponentNode, content_component_node);
  }
}

static void
gst_mpdparser_free_media_segment (GstMediaSegment * media_segment)
{
  if (media_segment) {
    g_slice_free (GstMediaSegment, media_segment);
  }
}

static void
gst_mpdparser_free_active_stream (GstActiveStream * active_stream)
{
  if (active_stream) {
    g_list_foreach (active_stream->segments,
        (GFunc) gst_mpdparser_free_media_segment, NULL);
    g_list_free (active_stream->segments);
    g_slice_free (GstActiveStream, active_stream);
  }
}

static gchar *
gst_mpdparser_get_mediaURL (GstSegmentURLNode *segmentURL)
{
  gchar *mediaURL;

  g_return_val_if_fail (segmentURL != NULL, NULL);
  g_return_val_if_fail (segmentURL->media != NULL, NULL);

  if (segmentURL->mediaRange) {
    gchar *range;
    range = g_strdup_printf ("?range=%llu-%llu", segmentURL->mediaRange->first_byte_pos, segmentURL->mediaRange->last_byte_pos);
    mediaURL = g_strconcat (segmentURL->media, range, NULL);
    g_free (range);
  } else {
    mediaURL = g_strdup (segmentURL->media);
  }

  return mediaURL;
}

static gchar *
gst_mpdparser_get_initializationURL (GstURLType *InitializationURL)
{
  gchar *mediaURL;

  g_return_val_if_fail (InitializationURL != NULL, NULL);
  g_return_val_if_fail (InitializationURL->sourceURL != NULL, NULL);

  if (InitializationURL->range) {
    gchar *range;
    range = g_strdup_printf ("?range=%llu-%llu", InitializationURL->range->first_byte_pos, InitializationURL->range->last_byte_pos);
    mediaURL = g_strconcat (InitializationURL->sourceURL, range, NULL);
    g_free (range);
  } else {
    mediaURL = g_strdup (InitializationURL->sourceURL);
  }

  return mediaURL;
}

static gchar *
gst_mpdparser_build_URL_from_template (const gchar *url_template,
    const gchar *id, guint number, guint bandwidth, guint time)
{
  static gchar default_format[] = "%01d";
  gchar **tokens, *token, *ret, *format;
  gint i, num_tokens;

  g_return_val_if_fail (url_template != NULL, NULL);
  tokens = g_strsplit_set (url_template, "$", -1);
  if (!tokens) {
    GST_WARNING ("Scan of URL template failed!");
    return NULL;
  }
  num_tokens = g_strv_length (tokens);

  for (i = 0; i < num_tokens; i++) {
    token = tokens[i];
    format = default_format;

    if (!g_strcmp0 (token, "RepresentationID")) {
      tokens[i] = g_strdup_printf ("%s", id);
      g_free (token);
    } else if (!strncmp (token, "Number", 6)) {
      if (strlen (token) > 6) {
        format = token + 6; /* format tag */
      }
      tokens[i] = g_strdup_printf (format, number);
      g_free (token);
    } else if (!strncmp (token, "Bandwidth", 9)) {
      if (strlen (token) > 9) {
        format = token + 9; /* format tag */
      }
      tokens[i] = g_strdup_printf (format, bandwidth);
      g_free (token);
    } else if (!strncmp (token, "Time", 4)) {
      if (strlen (token) > 4) {
        format = token + 4; /* format tag */
      }
      tokens[i] = g_strdup_printf (format, time);
      g_free (token);
    } else if (!g_strcmp0 (token, "")) {
      tokens[i] = g_strdup_printf ("%s", "$");
      g_free (token);
    }
  }

  ret = g_strjoinv (NULL, tokens);
  g_strfreev (tokens);

  return ret;
}

/* select a stream and extract the baseURL (if present) */
static gchar *
gst_mpdparser_parse_baseURL (GstMpdClient * client)
{
  GstActiveStream *stream;
  GstBaseURL *baseURL;
  GList *list;
  static gchar *baseURL_array[5];
  static gchar empty[] = "";
  gchar *ret = NULL;

  /* select stream TODO: support multiple streams */
  g_return_val_if_fail (client != NULL, empty);
  g_return_val_if_fail (client->active_streams != NULL, empty);
  stream = g_list_nth_data (client->active_streams, client->stream_idx);
  g_return_val_if_fail (stream != NULL, empty);

  baseURL_array[0] = baseURL_array[1] = baseURL_array[2] = baseURL_array[3] = empty;
  baseURL_array[4] = NULL;

  /* FIXME: this simple implementation is not fully compliant with RFC 3986 */
  if ((list = client->mpd_node->BaseURLs) != NULL) {
    baseURL = g_list_nth_data (list, stream->baseURL_idx);
    if (!baseURL) {
      baseURL = list->data;
    }
    baseURL_array[0] = baseURL->baseURL;
  }
  if ((list = client->cur_period->BaseURLs) != NULL) {
    baseURL = g_list_nth_data (list, stream->baseURL_idx);
    if (!baseURL) {
      baseURL = list->data;
    }
    baseURL_array[1] = baseURL->baseURL;
  }
  if ((list = stream->cur_adapt_set->BaseURLs) != NULL) {
    baseURL = g_list_nth_data (list, stream->baseURL_idx);
    if (!baseURL) {
      baseURL = list->data;
    }
    baseURL_array[2] = baseURL->baseURL;
  }
  if ((list = stream->cur_representation->BaseURLs) != NULL) {
    baseURL = g_list_nth_data (list, stream->baseURL_idx);
    if (!baseURL) {
      baseURL = list->data;
    }
    baseURL_array[3] = baseURL->baseURL;
  }

  ret = g_strjoinv (NULL, baseURL_array);
  /* get base URI from MPD file URI, if the "http" scheme is missing */
  if (client->mpd_uri != NULL && strncmp (ret, "http://", 7) != 0) {
    gchar *last_sep, *tmp1, *tmp2;
    last_sep = strrchr (client->mpd_uri, '/');
    if (last_sep) {
      tmp1 = g_strndup (client->mpd_uri, last_sep - client->mpd_uri + 1);
      tmp2 = ret;
      GST_DEBUG ("Got base URI from MPD file URI %s", tmp1);
      ret = g_strconcat (tmp1, tmp2, NULL);
      g_free (tmp1);
      g_free (tmp2);
    }
  }

  GST_DEBUG ("selected baseURL with index %d: %s", stream->baseURL_idx, ret);

  return ret;
}

/*****************************/
/******* API functions *******/
/*****************************/

GstMpdClient *gst_mpd_client_new ()
{
  GstMpdClient * client;

  client = g_new0 (GstMpdClient, 1);
  client->lock = g_mutex_new ();

  return client;
}

void gst_mpd_client_free (GstMpdClient * client)
{
  g_return_if_fail (client != NULL);

  if (client->mpd_node)
    gst_mpdparser_free_mpd_node (client->mpd_node);

  if (client->active_streams) {
    g_list_foreach (client->active_streams,
        (GFunc) gst_mpdparser_free_active_stream, NULL);
    g_list_free (client->active_streams);
  }

  if (client->lock)
    g_mutex_free (client->lock);

  g_free (client->mpd_uri);

  g_free (client);
}

gboolean
gst_mpd_parse (GstMpdClient * client, const gchar * data, gint size)
{
  if (data) {
    xmlDocPtr doc;
    xmlNode *root_element = NULL;

    GST_DEBUG ("MPD file fully buffered, start parsing...");

    /* parse the complete MPD file into a tree (using the libxml2 default parser API) */

    /* this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used
     */
    LIBXML_TEST_VERSION

    /* parse "data" into a document (which is a libxml2 tree structure xmlDoc) */
    doc = xmlReadMemory (data, size, "noname.xml", NULL, 0);
    if (doc == NULL) {
      GST_ERROR ("failed to parse the MPD file");
      return FALSE;
    } else {
      /* get the root element node */
      root_element = xmlDocGetRootElement (doc);

      if (root_element->type != XML_ELEMENT_NODE
          || xmlStrcmp (root_element->name, (xmlChar *) "MPD") != 0) {
        GST_ERROR ("can not find the root element MPD, failed to parse the MPD file");
      } else {
        /* now we can parse the MPD root node and all children nodes, recursively */
        gst_mpdparser_parse_root_node (client, root_element);
      }
      /* free the document */
      xmlFreeDoc (doc);
      /* cleanup function for the XML library */
      xmlCleanupParser ();
      /* dump XML library memory for debugging */
      xmlMemoryDump ();
    }

    return TRUE;
  }

  return FALSE;
}

const gchar *
gst_mpdparser_get_baseURL (GstMpdClient * client)
{
  GstActiveStream *stream;

  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (client->active_streams != NULL, NULL);
  stream = g_list_nth_data (client->active_streams, client->stream_idx);
  g_return_val_if_fail (stream != NULL, NULL);

  return stream->baseURL;
}

GstMediaSegment *
gst_mpdparser_get_chunk_by_index (GstMpdClient * client, guint indexStream, guint indexChunk)
{
  GstActiveStream *stream;

  /* select stream */
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (client->active_streams != NULL, NULL);
  stream = g_list_nth_data (client->active_streams, indexStream);
  g_return_val_if_fail (stream != NULL, NULL);

  return (GstMediaSegment *) g_list_nth_data (stream->segments, indexChunk);
}

gboolean
gst_mpd_client_setup_representation (GstMpdClient * client, GstActiveStream *stream, GstRepresentationNode *representation)
{
  GList *rep_list;
  GstMediaSegment *media_segment = NULL;
  GstClockTime PeriodStart = 0, PeriodEnd, start_time, duration;
  guint i;

  rep_list = stream->cur_adapt_set ? stream->cur_adapt_set->Representations : client->cur_period->Representations;
  stream->cur_representation = representation;
  stream->representation_idx = g_list_index (rep_list, representation);

  /* clean the old segment list, if any */
  if (stream->segments) {
    g_list_foreach (stream->segments,
        (GFunc) gst_mpdparser_free_media_segment, NULL);
    g_list_free (stream->segments);
    stream->segments = NULL;
  }

  if (client->cur_period->start != -1) {
    PeriodStart = client->cur_period->start * GST_MSECOND;
  }
  if (client->cur_period->duration != -1) {
    PeriodEnd = client->cur_period->duration * GST_MSECOND;
  } else {
    GstPeriodNode *next_period_node = gst_mpdparser_get_next_period (client->mpd_node->Periods, client->cur_period);
    if (next_period_node) {
      PeriodEnd = next_period_node->start * GST_MSECOND;
    } else {
      PeriodEnd = client->mpd_node->mediaPresentationDuration * GST_MSECOND;
    }
  }

  GST_LOG ("Building segment list for Period from %" GST_TIME_FORMAT " to %"
      GST_TIME_FORMAT, GST_TIME_ARGS (PeriodStart), GST_TIME_ARGS (PeriodEnd));

  if (representation->SegmentBase != NULL) {
    GList *SegmentURL;

    /* get the first segment_base of the selected representation */
    if ((stream->cur_segment_base =
            gst_mpdparser_get_segment_base (client->cur_period, stream->cur_adapt_set, representation)) == NULL) {
      GST_WARNING ("No valid SegmentBase node in the MPD file, aborting...");
      return FALSE;
    }

    /* get the first segment_list of the selected representation */
    if ((stream->cur_segment_list =
            gst_mpdparser_get_first_segment_list (representation->SegmentList)) == NULL) {
      GST_WARNING ("No valid SegmentList node in the MPD file, aborting...");
      return FALSE;
    }

    /* build the list of GstMediaSegment nodes from the SegmentList node */
    SegmentURL = stream->cur_segment_list->SegmentURL;
    if (SegmentURL == NULL) {
      GST_WARNING ("No valid list of SegmentURL nodes in the MPD file, aborting...");
      return FALSE;
    }

    /* build segment list */
    i = stream->cur_segment_list->MultSegBaseType->startNumber;
    start_time = 0;
    duration = gst_mpd_client_get_target_duration (client);

    while (SegmentURL) {
      media_segment = g_slice_new0 (GstMediaSegment);
      if (media_segment == NULL) {
        GST_WARNING ("Allocation of GstMediaSegment struct failed!");
        return FALSE;
      }
      stream->segments = g_list_append (stream->segments, media_segment);
      /* TODO: support SegmentTimeline */
      media_segment->SegmentURL = (GstSegmentURLNode *) SegmentURL->data;
      media_segment->number = i;
      media_segment->start_time = start_time;
      media_segment->duration = duration;
      i++;
      start_time += duration;
      SegmentURL = g_list_next (SegmentURL);
    }
  } else {
    if (representation->SegmentTemplate != NULL) {
      stream->cur_seg_template = representation->SegmentTemplate;
    } else if (stream->cur_adapt_set->SegmentTemplate != NULL) {
      stream->cur_seg_template = stream->cur_adapt_set->SegmentTemplate;
    } else if (client->cur_period->SegmentTemplate != NULL) {
      stream->cur_seg_template = client->cur_period->SegmentTemplate;
    }

    if (stream->cur_seg_template == NULL || stream->cur_seg_template->MultSegBaseType == NULL) {
      GST_WARNING ("Can not parse segment informations from MPD file due to incompatible syntax, aborting...");
      return FALSE;
    }

    /* build segment list */
    i = stream->cur_seg_template->MultSegBaseType->startNumber;
    start_time = 0;
    duration = gst_mpd_client_get_target_duration (client);

    GST_LOG ("using template %s", stream->cur_seg_template->media);
    while (PeriodStart + start_time < PeriodEnd) {
      media_segment = g_slice_new0 (GstMediaSegment);
      if (media_segment == NULL) {
        GST_WARNING ("Allocation of GstMediaSegment struct failed!");
        return FALSE;
      }
      stream->segments = g_list_append (stream->segments, media_segment);
      /* TODO: support SegmentTimeline */
      media_segment->number = i;
      media_segment->start_time = start_time;
      media_segment->duration = duration;
      i++;
      start_time += duration;
    }
  }

  if (media_segment) {
    /* fix duration of last segment */
    if (media_segment->start_time + media_segment->duration > PeriodEnd) {
      media_segment->duration = PeriodEnd - media_segment->start_time;
      GST_LOG ("Fixed duration of last segment: %" GST_TIME_FORMAT, GST_TIME_ARGS (media_segment->duration));
    }
    GST_LOG ("Built a list of %d segments", media_segment->number);
  }

  g_free (stream->baseURL);
  stream->baseURL = gst_mpdparser_parse_baseURL (client);

  return TRUE;
}

gboolean
gst_mpd_client_setup_streaming (GstMpdClient * client,
    GstStreamMimeType mimeType, gchar* lang)
{
  GstActiveStream *stream;
  GstAdaptationSetNode *adapt_set;
  GstRepresentationNode *representation;
  GList *rep_list;

  /* select a new period */
  if (!client->cur_period)
  if ((client->cur_period =
          gst_mpdparser_get_next_period (client->mpd_node->Periods, client->cur_period)) == NULL) {
    GST_WARNING ("No valid Period node in the MPD file, aborting...");
    return FALSE;
  }

  switch (mimeType) {
    case GST_STREAM_VIDEO:
      /* select the adaptation set for the video pipeline */
      adapt_set =
          gst_mpdparser_get_adapt_set_with_mimeType_and_idx (client->
          cur_period->AdaptationSets, "video", 0);
      /* if we found no 'video' adaptation set, just get the first one */
      if (!adapt_set)
        adapt_set =
            gst_mpdparser_get_first_adapt_set (client->cur_period->
            AdaptationSets);
      /* retrive the list of representations */
      if (adapt_set != NULL)
        rep_list = adapt_set->Representations;
      else
        rep_list = client->cur_period->Representations;
      if (!rep_list) {
        GST_WARNING ("Can not retrieve any representation, aborting...");
        return FALSE;
      }
      break;
    case GST_STREAM_AUDIO:
#if 0
      if (g_strcmp0 (client->audio_lang, "none") == 0) {
        GST_INFO ("Audio stream disabled");
        return FALSE;
      }
#endif
      adapt_set =
          gst_mpdparser_get_first_adapt_set_with_mimeType_and_lang (client->
          cur_period->AdaptationSets, "audio", lang);
      /* if we did not found the requested audio language, get the first one */
      if (!adapt_set)
        adapt_set =
            gst_mpdparser_get_first_adapt_set_with_mimeType (client->
            cur_period->AdaptationSets, "audio");
      if (!adapt_set) {
        GST_INFO ("No audio adaptation set found");
        return FALSE;
      }
      rep_list = adapt_set->Representations;
      if (!rep_list) {
        GST_WARNING ("Can not retrieve any representation, aborting...");
        return FALSE;
      }
      break;
    case GST_STREAM_APPLICATION:
#if 0
      if (g_strcmp0 (client->subtitle_lang, "none") == 0) {
        GST_INFO ("Subtitles pipeline disabled");
        return FALSE;
      }
#endif
      adapt_set =
          gst_mpdparser_get_first_adapt_set_with_mimeType_and_lang (client->
          cur_period->AdaptationSets, "application", lang);
      /* if we did not found the requested subtitles language, get the first one */
      if (!adapt_set)
        adapt_set =
            gst_mpdparser_get_first_adapt_set_with_mimeType (client->
            cur_period->AdaptationSets, "application");
      if (!adapt_set) {
        GST_INFO ("No application adaptation set found");
        return FALSE;
      }
      rep_list = adapt_set->Representations;
      if (!rep_list) {
        GST_WARNING ("Can not retrieve any representation, aborting...");
        return FALSE;
      }
      break;
    default:
      GST_WARNING ("Unsupported mimeType %d", mimeType);
      return FALSE;
  }

  stream = g_slice_new0 (GstActiveStream);
  if (stream == NULL) {
    GST_WARNING ("Allocation of active stream struct failed!");
    return FALSE;
  }
  client->active_streams = g_list_append (client->active_streams, stream);

  stream->baseURL_idx = 0;
  stream->mimeType = mimeType;
  stream->representation_idx = 0;
  stream->segment_idx = 0;
  stream->cur_adapt_set = adapt_set;

  /* retrive representation list */
  if (stream->cur_adapt_set != NULL)
    rep_list = stream->cur_adapt_set->Representations;
  else
    rep_list = client->cur_period->Representations;

#if 0
  /* fast start */
  representation =
      gst_mpdparser_get_representation_with_max_bandwidth (rep_list,
      stream->max_bandwidth);

  if (!representation) {
    GST_WARNING ("Can not retrieve a representation with the requested bandwidth");
    representation =
        gst_mpdparser_get_lowest_representation (rep_list);
  }
#else
  /* slow start */
  representation =
      gst_mpdparser_get_lowest_representation (rep_list);
#endif

  if (!representation) {
    GST_WARNING ("No valid representation in the MPD file, aborting...");
    return FALSE;
  }

  if (!gst_mpd_client_setup_representation (client, stream, representation))
    return FALSE;

  GST_INFO ("Successfully setup the download pipeline for mimeType %d", mimeType);

  return TRUE;
}

gboolean
gst_mpd_client_get_next_fragment (GstMpdClient * client,
    guint indexStream, gboolean *discontinuity, const gchar **uri,
    GstClockTime *duration, GstClockTime *timestamp)
{
  GstActiveStream *stream = NULL;
  GstMediaSegment *currentChunk;
  gchar *mediaURL = NULL;

  /* select stream */
  g_return_val_if_fail (client != NULL, FALSE);
  g_return_val_if_fail (client->active_streams != NULL, FALSE);
  stream = g_list_nth_data (client->active_streams, indexStream);
  g_return_val_if_fail (stream != NULL, FALSE);
  g_return_val_if_fail (stream->cur_representation != NULL, FALSE);
  g_return_val_if_fail (discontinuity != NULL, FALSE);

  GST_MPD_CLIENT_LOCK (client);
  GST_DEBUG ("Looking for fragment sequence chunk %d", stream->segment_idx);

  currentChunk = gst_mpdparser_get_chunk_by_index (client, indexStream, stream->segment_idx);
  if (currentChunk == NULL) {
    GST_MPD_CLIENT_UNLOCK (client);
    return FALSE;
  }

  if (currentChunk->SegmentURL != NULL) {
    mediaURL = gst_mpdparser_get_mediaURL (currentChunk->SegmentURL);
  } else if (stream->cur_seg_template != NULL) {
    mediaURL = gst_mpdparser_build_URL_from_template (stream->cur_seg_template->media,
        stream->cur_representation->id, currentChunk->number, stream->cur_representation->bandwidth, 0);
  }
  gst_mpd_client_get_current_position (client, timestamp);
  *discontinuity = stream->segment_idx != currentChunk->number;
  stream->segment_idx += 1;
  if (strncmp (mediaURL, "http://", 7) != 0) {
    *uri = g_strconcat (gst_mpdparser_get_baseURL (client), mediaURL, NULL);
    g_free (mediaURL);
  } else {
    *uri = mediaURL;
  }
  *duration = gst_mpd_client_get_target_duration (client);
  GST_MPD_CLIENT_UNLOCK (client);

  GST_DEBUG ("Loading chunk with URL %s", *uri);

  return TRUE;
}

gboolean
gst_mpd_client_get_next_header (GstMpdClient * client, const gchar **uri, guint stream_idx)
{
  GstActiveStream *stream = NULL;

  /* select stream */
  g_return_val_if_fail (client != NULL, FALSE);
  g_return_val_if_fail (client->active_streams != NULL, FALSE);
  /* TODO: support multiple streams */
  stream = g_list_nth_data (client->active_streams, stream_idx);
  g_return_val_if_fail (stream != NULL, FALSE);
  g_return_val_if_fail (stream->cur_representation != NULL, FALSE);

  GST_DEBUG ("Looking for current representation header");

  GST_MPD_CLIENT_LOCK (client);
  *uri = NULL;
  if (stream->cur_segment_base && stream->cur_segment_base->Initialization) {
    *uri = gst_mpdparser_get_initializationURL(stream->cur_segment_base->Initialization);
  } else if (stream->cur_seg_template) {
    *uri = gst_mpdparser_build_URL_from_template (stream->cur_seg_template->initialization,
        stream->cur_representation->id, 0, stream->cur_representation->bandwidth, 0);
  }
  GST_MPD_CLIENT_UNLOCK (client);

  return *uri == NULL ? FALSE : TRUE;
}

void
gst_mpd_client_get_current_position (GstMpdClient * client,
    GstClockTime * timestamp)
{
  GstActiveStream *stream;
  GstClockTime duration;

  *timestamp = GST_CLOCK_TIME_NONE;

  /* select stream TODO: support multiple streams */
  stream = g_list_nth_data (client->active_streams, client->stream_idx);
  g_return_if_fail (stream != NULL);

  duration = gst_mpd_client_get_target_duration (client);
  *timestamp = duration == GST_CLOCK_TIME_NONE ? GST_CLOCK_TIME_NONE : stream->segment_idx * duration;
}

GstClockTime
gst_mpd_client_get_duration (GstMpdClient * client)
{
  GstClockTime duration;

  g_return_val_if_fail (client != NULL, GST_CLOCK_TIME_NONE);

  GST_MPD_CLIENT_LOCK (client);
  duration = client->mpd_node->mediaPresentationDuration * GST_MSECOND;
  /* We can only get the duration for on-demand streams */
  if (!duration) {
    duration = GST_CLOCK_TIME_NONE;
  }
  GST_MPD_CLIENT_UNLOCK (client);

  return duration;
}

GstClockTime
gst_mpd_client_get_target_duration (GstMpdClient * client)
{
  GstActiveStream *stream;
  GstMultSegmentBaseType *base = NULL;
  GstClockTime duration;
  guint timescale;

  g_return_val_if_fail (client != NULL, GST_CLOCK_TIME_NONE);
  /* select stream TODO: support multiple streams */
  stream = g_list_nth_data (client->active_streams, client->stream_idx);
  g_return_val_if_fail (stream != NULL, GST_CLOCK_TIME_NONE);

  if (stream->cur_segment_list) {
    base = stream->cur_segment_list->MultSegBaseType;
  } else if (stream->cur_seg_template) {
    base = stream->cur_seg_template->MultSegBaseType;
  }

  g_return_val_if_fail (base != NULL, GST_CLOCK_TIME_NONE);
  g_return_val_if_fail (base->SegBaseType != NULL, GST_CLOCK_TIME_NONE);
  duration = base->duration * GST_SECOND;
  timescale = base->SegBaseType->timescale;

  if (timescale > 1)
    duration /= timescale;

  return duration;
}

gboolean
gst_mpd_client_is_live (GstMpdClient * client)
{
 return client->mpd_node->type == GST_MPD_FILE_TYPE_DYNAMIC;
}

guint gst_mpdparser_get_nb_active_stream (GstMpdClient *client){

 g_return_val_if_fail (client != NULL, FALSE);
 return g_list_length (client->active_streams);
}

guint gst_mpdparser_get_nb_adaptationSet(GstMpdClient *client){

 g_return_val_if_fail (client != NULL, FALSE);
 g_return_val_if_fail (client->cur_period != NULL, FALSE);
 g_return_val_if_fail (client->cur_period->AdaptationSets != NULL, FALSE);
 return g_list_length (client->cur_period->AdaptationSets);
}

GstActiveStream *gst_mpdparser_get_active_stream_by_index (GstMpdClient *client, gint stream_idx){

 g_return_val_if_fail (client != NULL, FALSE);
 g_return_val_if_fail (client->active_streams != NULL, FALSE);
 return g_list_nth_data (client->active_streams,
            stream_idx);
}

guint gst_mpd_client_get_width_of_video_current_stream (GstMpdClient *client, GstActiveStream *stream){

 g_return_val_if_fail (stream != NULL, FALSE);
 return stream->cur_representation->RepresentationBase->width;
}

guint gst_mpd_client_get_height_of_video_current_stream (GstMpdClient *client, GstActiveStream *stream){

 g_return_val_if_fail (stream != NULL, FALSE);
 return stream->cur_representation->RepresentationBase->height;
}

guint gst_mpd_client_get_rate_of_audio_current_stream (GstMpdClient *client, GstActiveStream *stream){

 g_return_val_if_fail (stream != NULL, FALSE);
 return (guint) stream->cur_representation->RepresentationBase->audioSamplingRate;
}

guint gst_mpd_client_get_num_channels_of_audio_current_stream (GstMpdClient *client, GstActiveStream *stream){

 g_return_val_if_fail (stream != NULL, FALSE);
 /* TODO*/
 return 1;
}

guint
gst_mpdparser_get_nb_audio_adapt_set(GList *  AdaptationSets)
{
 GList *list;
 GstAdaptationSetNode *adapt_set;
 guint nb_adapatation_set = 0;
 gchar *this_mimeType = "audio";
 gchar *mimeType = NULL;
 if (AdaptationSets == NULL)
	return 0;

 for (list = g_list_first (AdaptationSets); list; list = g_list_next (list)) {
	 adapt_set = (GstAdaptationSetNode *) list->data;
	 if (adapt_set) {
		 GstRepresentationNode *rep;
		 rep =
				gst_mpdparser_get_lowest_representation (adapt_set->Representations);
		 if (rep->RepresentationBase)
			 mimeType = rep->RepresentationBase->mimeType;
		 if (!mimeType && adapt_set->RepresentationBase) {
			 mimeType = adapt_set->RepresentationBase->mimeType;
		 }
		 if (strncmp_ext (mimeType, this_mimeType) == 0)
			 nb_adapatation_set++;
	 }
 }
 return nb_adapatation_set;
}

void gst_mpdparser_get_list_of_audio_language(GList** lang, GList *  AdaptationSets)
{
 GList *list;
 GstAdaptationSetNode *adapt_set;
 gchar *this_mimeType = "audio";
 gchar *mimeType = NULL;
 if (AdaptationSets == NULL)
	 return ;

 for (list = g_list_first (AdaptationSets); list; list = g_list_next (list)) {
	 adapt_set = (GstAdaptationSetNode *) list->data;
	 if (adapt_set) {
		 gchar *this_lang = adapt_set->lang;
		 GstRepresentationNode *rep;
		 rep =
			 	gst_mpdparser_get_lowest_representation (adapt_set->Representations);
		 if (rep->RepresentationBase)
		 	mimeType = rep->RepresentationBase->mimeType;
		 if (!mimeType && adapt_set->RepresentationBase) {
			 mimeType = adapt_set->RepresentationBase->mimeType;
		 }

		 if (strncmp_ext (mimeType, this_mimeType) == 0){
		  if(this_lang){
			  *lang = g_list_append (*lang, this_lang);
		  }
	 	}
 	}
 }
}
