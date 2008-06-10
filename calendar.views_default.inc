<?php
//$Id$
function calendar_views_default_views() {
 
$view = new view;
$view->name = 'calendar';
$view->description = 'A multi-dimensional calendar view with back/next navigation.';
$view->tag = 'Calendar';
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
  'arguments' => array(
    'date_argument' => array(
      'default_action' => 'default',
      'style_plugin' => 'default_summary',
      'style_options' => array(),
      'wildcard' => 'all',
      'wildcard_substitution' => 'All',
      'title' => '',
      'default_argument_type' => 'date',
      'default_argument' => '',
      'validate_type' => 'none',
      'validate_fail' => 'not found',
      'date_fields' => array(
        'node.changed' => 'node.changed',
      ),
      'date_method' => 'OR',
      'granularity' => 'month',
      'id' => 'date_argument',
      'table' => 'node',
      'field' => 'date_argument',
      'relationship' => 'none',
      'default_argument_user' => 0,
      'default_argument_fixed' => '',
      'default_argument_php' => '',
      'validate_argument_node_type' => array(
      ),
      'validate_argument_node_access' => 0,
      'validate_argument_nid_type' => 'nid',
      'validate_argument_vocabulary' => array(
      ),
      'validate_argument_type' => 'tid',
      'validate_argument_php' => '',
    ),
  ),
  'style_plugin' => 'calendar_nav',
  'style_options' => array(),
  'items_per_page' => 0,
  'offset' => 0,
  'fields' => array(
    'title' => array(
      'label' => 'Title',
      'link_to_node' => 1,
      'exclude' => 0,
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'relationship' => 'none',
    ),
  ),
);
$view->display['default'] = $display;
  $display = new views_display;
  $display->id = 'calendar';
  $display->display_title = 'Calendar page';
  $display->display_plugin = 'calendar';
  $display->position = '2';
  $display->display_options = array(
  'path' => 'calendar',
);
$view->display['calendar'] = $display;
  $display = new views_display;
  $display->id = 'calendar_year';
  $display->display_title = 'Year view';
  $display->display_plugin = 'calendar_year';
  $display->position = '3';
  $display->display_options = array(
  'displays' => array(
    'calendar' => 'calendar',
    'default' => 0,
    'calendar_block' => 0,
  ),
  'style_plugin' => 'calendar_style',
  'defaults' => array(
    'style_plugin' => FALSE,
    'style_options' => FALSE,
    'row_plugin' => FALSE,
    'row_options' => FALSE,
  ),
  'style_options' => array(
    'display_type' => 'year',
    'name_size' => '1',
  ),
  'row_plugin' => 'fields',
  'row_options' => array(),
  'attachment_position' => 'after',
);
$view->display['calendar_year'] = $display;
  $display = new views_display;
  $display->id = 'calendar_month';
  $display->display_title = 'Month view';
  $display->display_plugin = 'calendar_month';
  $display->position = '4';
  $display->display_options = array(
  'displays' => array(
    'calendar' => 'calendar',
    'default' => 0,
    'calendar_block' => 0,
  ),
  'style_plugin' => 'calendar_style',
  'defaults' => array(
    'style_plugin' => FALSE,
    'style_options' => FALSE,
    'row_plugin' => FALSE,
    'row_options' => FALSE,
  ),
  'style_options' => array(
    'display_type' => 'month',
    'name_size' => '99',
  ),
  'row_plugin' => 'fields',
  'row_options' => array(),
  'attachment_position' => 'after',
);
$view->display['calendar_month'] = $display;
  $display = new views_display;
  $display->id = 'calendar_day';
  $display->display_title = 'Day view';
  $display->display_plugin = 'calendar_day';
  $display->position = '5';
  $display->display_options = array(
  'displays' => array(
    'calendar' => 'calendar',
    'default' => 0,
    'calendar_block' => 0,
  ),
  'style_plugin' => 'calendar_style',
  'defaults' => array(
    'style_plugin' => FALSE,
    'style_options' => FALSE,
    'row_plugin' => FALSE,
    'row_options' => FALSE,
  ),
  'style_options' => array(
    'display_type' => 'day',
    'name_size' => '99',
  ),
  'row_plugin' => 'fields',
  'row_options' => array(),
  'attachment_position' => 'after',
);
$view->display['calendar_day'] = $display;
  $display = new views_display;
  $display->id = 'calendar_week';
  $display->display_title = 'Week view';
  $display->display_plugin = 'calendar_week';
  $display->position = '6';
  $display->display_options = array(
  'displays' => array(
    'calendar' => 'calendar',
    'default' => 0,
    'calendar_block' => 0,
  ),
  'style_plugin' => 'calendar_style',
  'defaults' => array(
    'style_plugin' => FALSE,
    'style_options' => FALSE,
    'row_plugin' => FALSE,
    'row_options' => FALSE,
  ),
  'style_options' => array(
    'display_type' => 'week',
    'name_size' => '99',
  ),
  'row_plugin' => 'fields',
  'row_options' => array(),
  'attachment_position' => 'after',
);
$view->display['calendar_week'] = $display;
  $display = new views_display;
  $display->id = 'calendar_block';
  $display->display_title = 'Calendar block';
  $display->display_plugin = 'calendar_block';
  $display->position = '7';
  $display->display_options = array();
$view->display['calendar_block'] = $display;
  $display = new views_display;
  $display->id = 'calendar_block_view';
  $display->display_title = 'Block view';
  $display->display_plugin = 'calendar_block_view';
  $display->position = '8';
  $display->display_options = array(
  'displays' => array(
    'calendar_block' => 'calendar_block',
    'default' => 0,
    'calendar' => 0,
  ),
  'style_plugin' => 'calendar_style',
  'defaults' => array(
    'style_plugin' => FALSE,
    'style_options' => FALSE,
    'row_plugin' => FALSE,
    'row_options' => FALSE,
  ),
  'style_options' => array(
    'display_type' => 'month',
    'name_size' => '1',
  ),
  'row_plugin' => 'fields',
  'row_options' => array(),
  'attachment_position' => 'after',
);
$view->display['calendar_block_view'] = $display;

  $views[$view->name] = $view;
  return $views;
}