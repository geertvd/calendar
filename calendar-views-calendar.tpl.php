<?php
// $Id$
/**
 * @file views-view-calendar.tpl.php
 * Template to display a view as a calendar.
 *
 * - $header: An array of header labels keyed by field id.
 * - $fields: An array of CSS IDs to use for each field id.
 * - $rows: An array of row items. Each row is an array of content
 *   keyed by field ID.
 * @ingroup views_templates
 */
dsm('Display: '. $display_type .': '. $min_date .' to '. $max_date);

//dsm($header);
dsm('Formatted results');
dsm($rows);
dsm('Raw results');
dsm($result);
?>
CALENDAR GOES HERE