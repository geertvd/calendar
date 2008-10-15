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
$view->disabled = TRUE; /* Edit this to true to make a default view disabled initially */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->override_option('fields', array(
  'title' => array(
    'label' => 'Title',
    'link_to_node' => 1,
    'exclude' => 0,
    'id' => 'title',
    'table' => 'node',
    'field' => 'title',
    'relationship' => 'none',
  ),
));
$handler->override_option('arguments', array(
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
      'page' => 0,
      'story' => 0,
    ),
    'validate_argument_node_access' => 0,
    'validate_argument_nid_type' => 'nid',
    'validate_argument_vocabulary' => array(
    ),
    'validate_argument_type' => 'tid',
    'validate_argument_php' => '',
  ),
));
$handler->override_option('filters', array(
  'status' => array(
    'operator' => '=',
    'value' => 1,
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'status',
    'table' => 'node',
    'field' => 'status',
    'relationship' => 'none',
  ),
));
$handler->override_option('access', array(
  'type' => 'none',
  'role' => array(),
  'perm' => '',
));
$handler->override_option('items_per_page', 0);
$handler->override_option('style_plugin', 'calendar_style');
$handler->override_option('style_options', array(
  'name_size' => '3',
  'with_weekno' => '0',
));

$handler = $view->new_display('calendar', 'Calendar page', 'calendar_1');
$handler->override_option('path', 'calendar');
$handler->override_option('style_plugin', 'calendar_nav');
$handler->override_option('style_options', array());
$handler->override_option('menu', array(
  'type' => 'none',
  'title' => '',
  'weight' => 0,
));
$handler->override_option('tab_options', array(
  'type' => 'none',
  'title' => '',
  'weight' => 0,
));
$handler->override_option('calendar_colors', array(
  'page' => '#ffff66',
  'story' => '#668cff',
));

$handler = $view->new_display('calendar_block', 'Calendar block', 'calendar_block_1');
$handler->override_option('style_plugin', 'calendar_nav');
$handler->override_option('style_options', array());

$handler = $view->new_display('calendar_period', 'Year view', 'calendar_period_1');
$handler->override_option('calendar_type', 'year');
$handler->override_option('style_plugin', 'calendar_style');
$handler->override_option('style_options', array(
  'display_type' => 'year',
  'name_size' => '1',
));
$handler->override_option('attachment_position', 'after');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', TRUE);
$handler->override_option('displays', array(
  'calendar_1' => 'calendar_1',
  'default' => 0,
  'calendar_block_1' => 0,
));

$handler = $view->new_display('calendar_period', 'Month view', 'calendar_period_2');
$handler->override_option('calendar_type', 'month');
$handler->override_option('style_plugin', 'calendar_style');
$handler->override_option('style_options', array(
  'display_type' => 'month',
  'name_size' => '99',
  'with_weekno' => '1',
  'date_fields' => NULL,
));
$handler->override_option('attachment_position', 'after');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', TRUE);
$handler->override_option('displays', array(
  'calendar_1' => 'calendar_1',
  'default' => 0,
  'calendar_block_1' => 0,
));

$handler = $view->new_display('calendar_period', 'Day view', 'calendar_period_3');
$handler->override_option('calendar_type', 'day');
$handler->override_option('style_plugin', 'calendar_style');
$handler->override_option('style_options', array(
  'display_type' => 'day',
  'name_size' => '99',
));
$handler->override_option('attachment_position', 'after');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', TRUE);
$handler->override_option('displays', array(
  'calendar_1' => 'calendar_1',
  'default' => 0,
  'calendar_block_1' => 0,
));

$handler = $view->new_display('calendar_period', 'Week view', 'calendar_period_4');
$handler->override_option('calendar_type', 'week');
$handler->override_option('style_plugin', 'calendar_style');
$handler->override_option('style_options', array(
  'display_type' => 'week',
  'name_size' => '99',
));
$handler->override_option('attachment_position', 'after');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', TRUE);
$handler->override_option('displays', array(
  'calendar_1' => 'calendar_1',
  'default' => 0,
  'calendar_block_1' => 0,
));

$handler = $view->new_display('calendar_period', 'Block view', 'calendar_period_5');
$handler->override_option('calendar_type', 'month');
$handler->override_option('style_plugin', 'calendar_style');
$handler->override_option('style_options', array(
  'display_type' => 'month',
  'name_size' => '1',
));
$handler->override_option('attachment_position', 'after');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', TRUE);
$handler->override_option('displays', array(
  'calendar_1' => 0,
  'default' => 0,
  'calendar_block_1' => 'calendar_block_1',
));

if (module_exists('calendar_ical')) {
$handler = $view->new_display('calendar_ical', 'iCal feed', 'calendar_ical_1');
$handler->override_option('style_plugin', 'ical');
$handler->override_option('style_options', array(
  'mission_description' => FALSE,
  'description' => '',
  'summary_field' => 'node_title',
  'description_field' => 'node_revisions_body',
  'location_field' => '',
));
$handler->override_option('path', 'calendar/%/ical');
$handler->override_option('menu', array(
  'type' => 'none',
  'title' => '',
  'weight' => 0,
));
$handler->override_option('tab_options', array(
  'type' => 'none',
  'title' => '',
  'weight' => 0,
));
$handler->override_option('displays', array(
  'calendar_1' => 'calendar_1',
  'default' => 0,
  'calendar_block_1' => 'calendar_block_1',
));

}

  $views[$view->name] = $view;
  return $views;
}