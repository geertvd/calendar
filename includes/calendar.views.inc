<?php

/**
 * @file
 * 
 * Creates calendar displays of Views results.
 */
/**
 * Implementation of hook_views_plugins
 */
function calendar_views_plugins() {
  $views_path = drupal_get_path('module', 'views');
  $module_path = drupal_get_path('module', 'calendar');
  $theme_path = $module_path;
  module_load_include('inc', 'calendar', 'theme/theme');

  $data = array(
    'module' => 'calendar', // This just tells our themes are elsewhere.

    'style' => array(
      'calendar_style' => array(
        'title' => t('Calendar'),
        'help' => t('Presents view results as a Calendar.'),
        'handler' => 'calendar_plugin_style',
        'path' => "$module_path/includes",
        'theme' => 'calendar_style',
        'theme file' => 'theme.inc',
        'theme path' => "$module_path/theme",
        'additional themes' => array(
          'calendar_year' => 'style',
          'calendar_day' => 'style',
          'calendar_week' => 'style',
          'calendar_mini' => 'style',
        ),
        'uses fields' => TRUE,
        'uses grouping' => FALSE,
        'uses row plugin' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
    'row' => array(
      'node_calendar' => array(
        'title' => t('Calendar Content'),
        'help' => t('Displays each selected node as a Calendar item.'),
        'handler' => 'calendar_plugin_row_node',
        'path' => "$module_path/includes",
        'theme' => 'calendar_row_node',
        'theme file' => 'theme.inc',
        'theme path' => "$module_path/theme",
        'base' => array('node'), // only works with 'node' as base.
        'uses options' => TRUE,
        'uses fields' => FALSE,
        'type' => 'normal',
      ),
    ),
  );
  return $data;
}
