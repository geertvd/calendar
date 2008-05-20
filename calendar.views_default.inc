<?php
//$Id$
function calendar_views_default_views() {
$view = new view;
$view->name = 'dates';
$view->description = 'Display Views results in a calendar.';
$view->tag = '';
$view->view_php = '';
$view->base_table = 'node';
$view->is_cacheable = '0';
$view->api_version = 2;
$view->disabled = FALSE; // Edit this to true to make a default view disabled initially
$view->display = array();
  $display = new views_display;
  $display->id = 'default';
  $display->display_title = 'Defaults';
  $display->display_plugin = 'default';
  $display->position = '1';
  $display->display_options = array(
  'style_plugin' => 'calendar',
  'style_options' => array(
    'date_fields' => array(
      '0' => 'node.changed',
    ),
    'default_display' => 'month',
    'year' => 'calendar',
    'month' => 'calendar',
    'week' => 'calendar',
    'day' => 'calendar',
    'calendar_time_format' => 'H:i',
    'calendar_day_header' => '1',
    'block_args' => '',
  ),
  'row_plugin' => 'fields',
  'row_options' => array(),
  'relationships' => array(),
  'fields' => array(
    'title' => array(
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'label' => 'Title',
      'relationship' => 'none',
      'link_to_node' => 1,
    ),
    'changed' => array(
      'id' => 'changed',
      'table' => 'node',
      'field' => 'changed',
      'label' => 'Updated date',
      'date_format' => 'medium',
      'custom_date_format' => '',
      'relationship' => 'none',
    ),
  ),
  'sorts' => array(),
  'arguments' => array(
    'calendar_date' => array(
      'id' => 'calendar_date',
      'table' => 'node',
      'field' => 'calendar_date',
      'default_action' => 'default',
      'style_plugin' => 'default_summary',
      'style_options' => array(
        'count' => TRUE,
        'override' => FALSE,
        'items_per_page' => 25,
      ),
      'wildcard' => 'all',
      'wildcard_substitution' => 'All',
      'title' => '',
      'default_argument_type' => 'calendar',
      'default_argument' => '',
      'validate_type' => 'none',
      'validate_fail' => 'not found',
      'date_fields' => array(
        'node.changed' => 'node.changed',
      ),
      'date_method' => 'OR',
      'relationship' => 'none',
      'default_argument_user' => 0,
      'default_argument_calendar' => 'Y-m',
      'default_argument_fixed' => '',
      'default_argument_php' => '',
      'validate_argument_node_type' => array(
        'page' => 0,
        'story' => 0,
        'test' => 0,
      ),
      'validate_argument_node_access' => 0,
      'validate_argument_nid_type' => 'nid',
      'validate_argument_vocabulary' => array(),
      'validate_argument_type' => 'tid',
      'validate_argument_php' => '',
    ),
  ),
  'filters' => array(),
  'items_per_page' => 10,
);
$view->display['default'] = $display;
  $display = new views_display;
  $display->id = 'page';
  $display->display_title = 'Page';
  $display->display_plugin = 'page';
  $display->position = '2';
  $display->display_options = array(
  'defaults' => array(
    'access' => TRUE,
    'title' => TRUE,
    'header' => TRUE,
    'header_format' => TRUE,
    'header_empty' => TRUE,
    'footer' => TRUE,
    'footer_format' => TRUE,
    'footer_empty' => TRUE,
    'empty' => TRUE,
    'empty_format' => TRUE,
    'use_ajax' => TRUE,
    'items_per_page' => TRUE,
    'offset' => TRUE,
    'use_pager' => TRUE,
    'pager_element' => TRUE,
    'use_more' => TRUE,
    'distinct' => TRUE,
    'link_display' => TRUE,
    'style_plugin' => TRUE,
    'style_options' => TRUE,
    'row_plugin' => TRUE,
    'row_options' => TRUE,
    'relationships' => TRUE,
    'fields' => TRUE,
    'sorts' => TRUE,
    'arguments' => TRUE,
    'filters' => TRUE,
  ),
  'relationships' => array(),
  'fields' => array(),
  'sorts' => array(),
  'arguments' => array(),
  'filters' => array(),
  'path' => 'calendar2',
);
$view->display['page'] = $display;


  $views[$view->name] = $view;
  return $views;
}