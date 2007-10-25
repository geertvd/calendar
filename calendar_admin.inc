<?php
/**
 * @file
 * This file contains administrative functions used only when setting up
 * the calendar and views_hooks() that are called infrequently and cached.
 * No need to parse all this code the rest of the time.
 */

/**
 * Function to return all possible calendar views page display types.
 * Implements hook_calendar_add_types($view) for other modules
 * to add calendar types.
 */
function _calendar_view_types($reset = FALSE) {
  static $types;
  if ($reset || !isset($types)) {
  	$types = array('calendar' => t('Calendar'));
    foreach (module_implements('_calendar_add_types') as $module) {
      $function = $module .'_calendar_add_types';
      $types += $function($view);
    }
  }
  return $types;
}

/**
 * Function to get information about all views that have calendar components.
 *
 * @return
 *   array with views that use calendar plugins or have calendar arguments.
 */
function _calendar_info() {
  $cid = 'calendar_views';
  cache_clear_all($cid, 'cache_views');
  $calendar_views = array();
  $calendar_types = calendar_view_types();
  $result = db_query("SELECT vid, name FROM {view_view} ORDER BY name");
  while ($v = db_fetch_object($result)) {
    $view = views_get_view($v->vid);
    $additions = array();
    $additions['vid'] = $view->vid;
    $additions['name'] = $view->name;
    $additions['url'] = $view->url;
    $additions['args'] = array();
    $additions['page'] = array_key_exists($view->page_type, $calendar_types);
    $additions['block'] = array_key_exists($view->block_type, $calendar_types);
    foreach ((array) $view->argument as $delta => $argument) {
      if (in_array($argument['type'], calendar_args())) {
        $additions['args'][$delta] = $argument['id'];
      }
    }
    if (!empty($additions['args']) || $additions['page'] || $additions['block']) {
      $calendar_views[$view->name] = $additions;
    }
  }
  views_load_cache();
  foreach (_views_get_default_views() as $view) {
    if (empty($view->disabled)) {
      $additions = array();
      $additions['vid'] = $view->vid;
      $additions['name'] = $view->name;
      $additions['url'] = $view->url;
      $additions['args'] = array();
      $additions['page'] = array_key_exists($view->page_type, $calendar_types);
      $additions['block'] = array_key_exists($view->block_type, $calendar_types);
      foreach ((array) $view->argument as $delta => $argument) {
        if (in_array($argument['type'], calendar_args())) {
          $additions['args'][$delta] = $argument['id'];
        }
      }
      if (!empty($additions['args']) || $additions['page'] || $additions['block']) {
        $calendar_views[$view->name] = $additions;
      }
    }
  }
  cache_set($cid, 'cache_views', serialize($calendar_views));
  return $calendar_views;
}

/**
 *  Identify all potential date/timestamp fields.
 *
 *  @return
 *   array with fieldname, type, and table
 */
function _calendar_fields() {
  $cid = 'calendar_fields';
  cache_clear_all($cid, 'cache_views');
  $delta = 0;
  $event_fields_processed = array();
  views_load_cache();
  foreach (_views_get_fields() as $name => $val) {
    $timestamp_fromto = array();
    $string_fromto = array();
    $tmp = explode('.', $name);
    $field_name = $val['content_field']['field_name'] ? $val['content_field']['field_name'] : $tmp[1];

    // We need to treat event_start and event_end as a single date, all other fields have
    // the same field_name for both start and end dates.
    $processed_name = strstr($field_name, 'event_') ? 'event' : $field_name;

    $type = '';
    // for cck fields, get the date type
    if ($val['content_field']['type'] == 'date' || $val['content_field']['type'] == 'datestamp') {
      $type = $val['content_field']['type'] == 'date' ? 'cck_string' : 'cck_timestamp';
    }
    // all other fields that use the views date handler are timestamps
    elseif ($val['handler'] == views_handler_field_dates()) {
      $type = 'timestamp';
    }
    // don't do anything if this is not a date field
    if ($type) {
      // dates with from and to dates need to handle both fields as one
      // add the from and to dates to the first one found and ignore the second
      // Handling for event fromto dates
      if (module_exists('event') && !in_array($processed_name, $event_fields_processed) && ($name == 'event.event_start' || $name == 'event.event_end')) {
        $timestamp_fromto = array('event.event_start', 'event.event_end');
        $offset_field = 'event.timezone';
        $tz_handling = variable_get('event_timezone_display', 'site');
        $event_fields_processed[] = $processed_name;
      }
      elseif ($val['content_field']['tz_handling']) {
        $tz_handling = $val['content_field']['tz_handling'];
        if ($tz_handling == 'date') $offset_field = $val['table'] .'.'. $val['content_db_info']['columns']['offset']['column'];
      }
      else {
        $tz_handling = 'site';
      }
      // Handling for cck fromto dates
      if (!in_array($processed_name, $event_fields_processed) && $val['content_field']['todate']) {
        switch ($val['content_field']['type']) {
        case 'datestamp':
          $timestamp_fromto = array($val['table'] .'.'. $field_name .'_value', $val['table'] .'.'. $field_name .'_value2');
          break;
        case 'date':
          $string_fromto = array($val['table'] .'.'. $field_name .'_value', $val['table'] .'.'. $field_name .'_value2');
          break;
        }
        $event_fields_processed[] = $processed_name;
      }
      // skip this step on second occurance of fromto date fields, if more than one exists in view
      if (!in_array($processed_name, $event_fields_processed) || $timestamp_fromto || $string_fromto) {
        // cck fields append a column name to the field, others do not
        // need a real field_name with no column name appended for cck date formatters
        $fields[$tmp[1]] = array(
          'type' => $type,
          'delta' => $delta,
          'label' => $val['name'],
          'granularity' => is_array($val['content_field']['granularity']) ? (array) array_keys($val['content_field']['granularity']) : array('Y', 'M', 'D', 'H', 'N'),
          'fullname' => $name,
          'table' => $tmp[0],
          'field' => $tmp[1],
          'field_name' => $field_name,
          'query_name' => str_replace('.', '_', $name),
          'timestamp_fromto' => $timestamp_fromto,
          'string_fromto' => $string_fromto,
          'tz_handling' => $tz_handling,
          'offset_field' => $offset_field,
        );
      }
    }
  }
  cache_set($cid, 'cache_views', serialize($fields));
  return $fields;
}

/**
 * Validate a view.
 */
function _calendar_views_validate($type, $view, $form) {
  // list (and table) modes require there to be at least 1 field active.
  if (is_array($view['field'])) {
    $fields = array_filter(array_keys($view['field']), 'is_numeric');
  }
  if (!$fields) {
    form_error($form["$type-info"][$type . '_type'], t('The Calendar View requires at least one field.'));
  }

  if (isset($view['field']['count'])) {
    $defaultsort = false;
    for ($i = 0; $i < $view['field']['count']; $i++) {
      if ($view['field'][$i]['defaultsort']) {
        if ($defaultsort) {
          form_error($form['field'][$i]['defaultsort'], t('You can only set on Default Sort on one field.'));
          break;
        }
        $defaultsort = true;
      }
    }
  }
  // Make sure all arguments are set to 'Display all values'.
  $arg_types = array();
  $cal_args = calendar_args();
  foreach ($view['argument'] as $delta => $argument) {
    if (in_array($argument['type'], $cal_args)) {
      $view['argument'][$delta]['argdefault'] = 2;
      if ($argument['argdefault'] != 2) {
        form_error($form['argument'][$delta]['argdefault'], t('Calendar arguments must be set to \'Display All Values\'.'));
      }
      $arg_types[] = $argument['type'];
    }
  }
  // Must have Year, Month, and Day or Year and Week calendar arguments.
  if (!in_array('calendar_year', $arg_types) && ((!in_array('calendar_month', $arg_types) && !in_array('calendar_day', $arg_types) || !in_array('calendar_week', $arg_types)))) {
    form_error($form['argument'], t('The Calendar requres as arguments Calendar: Year, Calendar: Month, and Calendar: Day, or Calendar: Year and Calendar: Week'));
  }
  // CCK date fields cannot use grouped handler.
  $calendar_fields = array_keys(calendar_fields());
  foreach ($view['field'] as $delta => $field) {
    if (in_array($field['field'], $calendar_fields) && $field['handler'] == 'content_views_field_handler_group') {
      form_error($form['field'][$delta]['handler'], t('Calendar CCK Date fields must be set to \'Do not group multiple values\'.'));
    }
  }
}

/**
 * Setup Calendar parameters.
 */
function _calendar_setup_form($view_name) {
	calendar_load_date_api();
	$view = views_load_view($view_name);
  $form = array();
  $time = mktime(1, 15, 0, 1, 1, date('Y', date_time()));
  $time_options = array(
    'G:i' => date('G:i', $time),
    'g:ia' => date('g:ia', $time),
    'g:iA' => date('g:iA', $time),
    'g:i a' => date('g:i a', $time),
    'g:i A' => date('g:i A', $time),
    'H:i' => date('H:i', $time),
    'h:ia' => date('h:ia', $time),
    'h:iA' => date('h:iA', $time),
    'h:i a' => date('h:i a', $time),
    'h:i A' => date('h:i A', $time),
    );

  $form['calendar_time_format'] = array(
    '#title' => t('Time format'),
    '#default_value' => variable_get('calendar_time_format_'. $view->name, 'H:i'),
    '#type' => 'select',
    '#options' => $time_options,
    '#description' => t('The format to use for the time-only date display.'),
    );
  $display_options = array('calendar' => t('Calendar'), 'table' => t('Table'), 'teasers' => t('Teasers'), 'nodes' => t('Full Nodes'), 'list' => t('List'));
  $display_format = variable_get('calendar_display_format_'. $view->name, array('year' => 'calendar', 'month' => 'calendar', 'week' => 'calendar', 'day' => 'calendar', 'block' => 'calendar'));
  $form['year'] = array(
    '#title' => t('Year display'),
    '#default_value' => $display_format['year'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['month'] = array(
    '#title' => t('Month display'),
    '#default_value' => $display_format['month'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['week'] = array(
    '#title' => t('Week display'),
    '#default_value' => $display_format['week'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['day'] = array(
    '#title' => t('Day display'),
    '#default_value' => $display_format['day'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['block'] = array(
    '#title' => t('Block display'),
    '#default_value' => $display_format['block'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['calendar_empty_arg'] = array(
    '#title' => t('Wildcard argument'),
    '#type' => 'textfield',
    '#default_value' => variable_get('calendar_empty_arg', 'all'),
    '#description' => t('A character or short text string to use for empty calendar arguments. For instance, \'all\' would create the url 2007/12/all to show all days in December of 2007. Note that non-ASCII characters will not display correctly in urls.')
    );
  $form['view_name'] = array(
    '#type' => 'hidden',
    '#value' => $view->name,
    );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),    );
  return $form;
}

/**
 * Save requested values.
 */
function _calendar_setup_form_submit($form_id, $form_values) {
  $view_name = $form_values['view_name'];
  $display_format = array();
  variable_set('calendar_empty_arg', check_plain($form_values['calendar_empty_arg']));
  foreach ($form_values as $value_name => $value) {
  	switch ($value_name) {
  		case 'calendar_time_format':
        variable_set('calendar_time_format_'. $view_name,  $value);
        break;
  		case 'year':
  		case 'month':
  		case 'week':
  		case 'day':
  		case 'block':
  			$display_format[$value_name] = $value;
  			break;
    }
  }
  variable_set('calendar_display_format_'. $view_name, $display_format);
}

/**
 *  Implementation of hook_views_style_plugins()
 */
function _calendar_views_style_plugins() {
  $plugins = array();
  $types = array('calendar' => t('Calendar'));
  foreach ($types as $name => $type) {
    $plugins[$name] = array(
      'name' => $type,
      'theme' => 'calendar_view_calendar',
      'summary_theme' => 'calendar_view_summary',
      'validate' => 'calendar_views_validate',
      'needs_fields' => TRUE,
      'needs_table_header' => FALSE,
      'even_empty' => TRUE,
      );
  }
  return $plugins;
}

/**
 *  Implementation of hook_views_arguments()
 */
function _calendar_views_arguments() {
  $arguments = array(
    'calendar_year' => array(
      'name' => t('Calendar: Year'),
      'handler' => 'calendar_handler_arg_year',
      'help' => t('Filter by the calendar year (YYYY).'),
    ),
    'calendar_month' => array(
      'name' => t('Calendar: Month'),
      'handler' => 'calendar_handler_arg_month',
      'help' => t("Filter by the calendar month (1-12). Place this argument after a 'Year' argument."),
    ),
    'calendar_day' => array(
      'name' => t('Calendar: Day'),
      'handler' => 'calendar_handler_arg_day',
      'help' => t("Filter by the calendar day (1-31). Place this argument after a 'Year' and a 'Month' argument."),
    ),
    'calendar_week' => array(
      'name' => t('Calendar: Week'),
      'handler' => 'calendar_handler_arg_week',
      'help' => t("Filter by the week number (1-52). Place this argument after a 'Year' argument and use a 'W' in front of the week number in the url."),
    ),
  );
  return $arguments;
}

function _calendar_views_default_views() {
  $view = new stdClass();
  $view->name = t('calendar');
  $view->description = t('Calendar view of any date field, add a date field to the view to use it.');
  $view->access = array ();
  $view->view_args_php = '';
  $view->page = TRUE;
  $view->page_title = t('Calendar');
  $view->page_header = "";
  $view->page_header_format = '1';
  $view->page_footer = "";
  $view->page_footer_format = '1';
  $view->page_empty = "";
  $view->page_empty_format = '1';
  $view->page_type = 'calendar';
  $view->url = t('calendar');
  $view->use_pager = FALSE;
  $view->nodes_per_page = '0';
  $view->block = TRUE;
  $view->block_title = t('Calendar');
  $view->block_header = "";
  $view->block_header_format = '1';
  $view->block_footer = "";
  $view->block_footer_format = '1';
  $view->block_empty = "";
  $view->block_empty_format = '1';
  $view->block_type = 'calendar';
  $view->nodes_per_block = '999';
  $view->block_more = '1';
  $view->block_use_page_header = FALSE;
  $view->block_use_page_footer = FALSE;
  $view->block_use_page_empty = FALSE;
  $view->disabled = TRUE;
  $view->sort = array ();
  $view->argument = array (
    array (
      'type' => 'calendar_year',
      'argdefault' => '2',
      'title' => '%1',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array (
      'type' => 'calendar_month',
      'argdefault' => '2',
      'title' => '%2',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array (
      'type' => 'calendar_day',
      'argdefault' => '2',
      'title' => '%3',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
  );
  $view->field = array (
    array (
      'tablename' => 'node',
      'field' => 'title',
      'label' => 'Title:',
      'handler' => 'views_handler_field_nodelink',
      'options' => 'link',
    ),
  );
  $view->filter = array (
    array (
      'tablename' => 'node',
      'field' => 'status',
      'operator' => '=',
      'options' => '',
      'value' => '1',
    ),
  );
  $view->exposed_filter = array (
  );
  $view->requires = array(node);
  $views[$view->name] = $view;
  return $views;
}
