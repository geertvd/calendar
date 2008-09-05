<?php
//$Id$
//views_include_handlers();
/**
 * Implementation of hook_views_plugins
 */
function calendar_ical_views_plugins() {
  $path = drupal_get_path('module', 'calendar');
  require_once "./$path/theme/theme.inc";

  $base = array(
    'file' => 'theme.inc',
    'path' => "$path/theme",
  );
  return array(
    'module' => 'calendar_ical', // This just tells our themes are elsewhere.
    'display' => array(
      'ical' => $base + array(
        'title' => t('iCal feed'),
        'help' => t('Display the view as an iCal feed.'),
        'handler' => 'calendar_plugin_display_ical',
        'uses hook menu' => TRUE,
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
        'admin' => t('iCal feed'),
        'help topic' => 'display-ical',
      ),
    ),
    'style' => array(
      'ical' => $base + array(
      'title' => t('iCal Feed'),
      'help' => t('Generates an iCal feed from a view.'),
      'handler' => 'calendar_plugin_style_ical',
      'theme' => 'calendar_view_ical',
      'uses row plugin' => FALSE,
      'uses fields' => TRUE,
      'uses row plugin' => FALSE,
      'uses options' => TRUE,
      'type' => 'ical',
      'even empty' => TRUE,
      ),
    ),
  );
}