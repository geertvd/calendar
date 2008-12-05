<?php
// $Id$
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
  $calendar_fields = calendar_fields();
  views_load_cache();
  foreach (_views_get_default_views() as $view) {
    if (empty($view->disabled)) {
      $additions = array();
      $additions['vid'] = $view->vid;
      $additions['name'] = $view->name;
      $additions['url'] = $view->url;
      $additions['args'] = array();
      foreach ((array) $view->field as $delta => $field) {
        if (in_array($field['field'], array_keys($calendar_fields))) {
          $additions['fields'][$delta] = $field['id'];
        }
      }
      if (!empty($additions['fields'])) {
        $additions['page'] = array_key_exists($view->page_type, $calendar_types);
        $additions['block'] = array_key_exists($view->block_type, $calendar_types);
        $additions['embed'] = $additions['page'] || $additions['block'];
      }
      else {
        $additions['page'] = FALSE;
        $additions['block'] = FALSE;
        $additions['embed'] = FALSE;
      }
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
  $result = db_query("SELECT vid, name FROM {view_view} ORDER BY name");
  while ($v = db_fetch_object($result)) {
    $view = views_get_view($v->vid);
    $additions = array();
    $additions['vid'] = $view->vid;
    $additions['name'] = $view->name;
    $additions['url'] = $view->url;
    $additions['args'] = array();
    foreach ((array) $view->field as $delta => $field) {
      if (in_array($field['field'], array_keys($calendar_fields))) {
        $additions['fields'][$delta] = $field['id'];
      }
    }
    if (!empty($additions['fields'])) {
      $additions['page'] = array_key_exists($view->page_type, $calendar_types);
      $additions['block'] = array_key_exists($view->block_type, $calendar_types);
      $additions['embed'] = $additions['page'] || $additions['block'];
    }
    else {
      $additions['page'] = FALSE;
      $additions['block'] = FALSE;
      $additions['embed'] = FALSE;
    }
    foreach ((array) $view->argument as $delta => $argument) {
      if (in_array($argument['type'], calendar_args())) {
        $additions['args'][$delta] = $argument['id'];
      }
    }
    if (!empty($additions['args']) || $additions['page'] || $additions['block']) {
      $calendar_views[$view->name] = $additions;
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
    $fromto = array();
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
    // Don't do anything if this is not a date field we can handle.
    if ($type && $name != 'event.event_start' && $name != 'event.event_end') {

      $fromto = array($name, $name);
      $tz_handling = 'site';
      $related_fields = array();
      $timezone_field = '';
      $delta_field = '';

      // dates with from and to dates need to handle both fields as one
      // add the from and to dates to the first one found and ignore the second

      // Handling for content field dates
      if ($val['content_field']['tz_handling']) {
        
        $tz_handling = $val['content_field']['tz_handling'];
        if ($tz_handling == 'date') {
          $offset_field = $val['table'] .'.'. $val['content_db_info']['columns']['offset']['column'];
          $timezone_field = $val['table'] .'.'. $field_name .'_timezone';
          $related_fields = array(
            $val['table'] .'.'. $field_name .'_value',
            $val['table'] .'.'. $field_name .'_value2',
            $val['table'] .'.'. $field_name .'_timezone',
            $val['table'] .'.'. $field_name .'_offset',
            $val['table'] .'.'. $field_name .'_offset2',
            );
        }
        else {
          $related_fields = array(
            $val['table'] .'.'. $field_name .'_value',
            $val['table'] .'.'. $field_name .'_value2',
          );          
        }
        if (!empty($val['content_field']['repeat'])) {
          $related_fields[] = $val['table'] .'.'. $field_name .'_rrule';
        }
      }
      // Get the delta value into the query.
      if (!empty($val['content_field']['multiple'])) {
        array_push($related_fields, $val['table'] .'.delta');
        $delta_field = $val['table'] .'_delta';
      }
      
      // Handling for cck fromto dates
      if (!in_array($processed_name, $event_fields_processed)) {
        switch ($val['content_field']['type']) {
          case 'datestamp':
            $fromto = array(
              $val['table'] .'.'. $field_name .'_value',
              $val['table'] .'.'. ($val['content_field']['todate'] ? $field_name .'_value2' : $field_name .'_value'),
              );
            break;
          case 'date':
            $fromto = array(
              $val['table'] .'.'. $field_name .'_value',
              $val['table'] .'.'. ($val['content_field']['todate'] ? $field_name .'_value2' : $field_name .'_value'),
              );
            break;
        }
        $event_fields_processed[] = $processed_name;
      }

       // skip this step on second occurrence of fromto date fields, if more than one exists in view
      // cck fields append a column name to the field, others do not
      // need a real field_name with no column name appended for cck date formatters
      switch ($type) {
        case 'cck_string':
          $sql_type = DATE_ISO;
          break;
        case 'cck_datetime':
          $sql_type = DATE_DATETIME;
          break;
        default:
          $sql_type = DATE_UNIX;
          break;
      }

      // skip this step on second occurrence of fromto date fields, if more than one exists in view
      if (!in_array($processed_name, $event_fields_processed) || $fromto) {
        // cck fields append a column name to the field, others do not
        // need a real field_name with no column name appended for cck date formatters
        $fields[$tmp[1]] = array(
          'type' => $type,
          'sql_type' => $sql_type,
          'delta' => $delta,
          'label' => $val['name'],
          'fullname' => $name,
          'table' => $tmp[0],
          'field' => $tmp[1],
          'field_name' => $field_name,
          'query_name' => str_replace('.', '_', $name),
          'fromto' => $fromto,
          'tz_handling' => $tz_handling,
          'offset_field' => $offset_field,
          'timezone_field' => $timezone_field,
          'delta_field' => $delta_field,
          'related_fields' => $related_fields,
        );
      }
    }
  }
  //cache_set($cid, 'cache_views', serialize($fields));
  return $fields;
}

/**
 * Validate a view.
 */
function _calendar_views_validate($type, $view, $form) {
  $calendar_fields = array_keys(calendar_fields());
  $fields = array();
  
  // list (and table) modes require there to be at least 1 field active.
  if (is_array($view['field'])) {
    foreach ($view['field'] as $key => $value) {
      if (is_numeric($key)) {
        if (in_array($view['field'][$key]['field'], $calendar_fields)) {
          $fields[] = $view['field'][$key]['field'];
        }
      }
    }
  }
  if ($form['field'] && !$fields) {
    form_error($form['field'], t('The Calendar View requires at least one date field.'));
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
    form_error($form['argument'], t('The Calendar requires as arguments Calendar: Year, Calendar: Month, and Calendar: Day, or Calendar: Year and Calendar: Week'));
  }
  // CCK date fields cannot use grouped handler.
  $calendar_fields = array_keys(calendar_fields());
  
  // Include content_views handlers as well as newer date_views handlers
  // so this will work until views are updated.
  $handlers = array(
    'content_views_field_handler_group', 
    'content_views_field_handler_first', 
    'content_views_field_handler_last', 
    'date_views_field_handler_group', 
    'date_views_field_handler_first', 
    'date_views_field_handler_last',
    );
  foreach ($view['field'] as $delta => $field) {
    if (in_array($field['field'], $calendar_fields) && in_array($field['handler'], $handlers)) {
      form_error($form['field'][$delta]['handler'], t('Calendar CCK Date fields must be set to \'Do not group multiple values\'.'));
    }
  }
}

function _calendar_settings_header($view) {
  if ($view->page && $view->page_title || $view->block && $view->block_title || $view->description) {
    $title = $view->page && $view->page_title ? $view->page_title : ($view->block && $view->block_title ? $view->block_title : '');
    return '<h2>'. $title . '</h2>' . (!empty($view->description) ? '<p><em>'. $view->description .'</em></p>' : '') . ($view->page && $view->url ? '<p><strong>URL:</strong> '. l($view->url, $view->url) .'</p>' : '');
  }  
}

/**
 * Setup Calendar parameters.
 */
function _calendar_setup_form($view_name) {
 $view = views_get_view($view_name);

  $form = array();
  $time = mktime(1, 15, 0, 1, 1, date('Y', time()));
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
  $form['markup'] = array(
    '#type' => 'markup',
    '#value' => _calendar_settings_header($view),
    );
  $form['calendar_time_format'] = array(
    '#title' => t('Time format'),
    '#default_value' => variable_get('calendar_time_format_'. $view->name, 'H:i'),
    '#type' => 'select',
    '#options' => $time_options,
    '#description' => t('The format to use for the time-only date display.'),
    );
  $form['calendar_day_header'] = array(
    '#title' => t('Mini day name size'),
    '#default_value' => variable_get('calendar_day_header_'. $view->name, 1),
    '#type' => 'select',
    '#options' => array(1 => t('First letter of name'), 2 => t('First two letters of name'), 3 => t('Abbreviated name'), 99 => t('Full name')),
    '#description' => t('The way day of week names should be displayed in a mini calendar.'),
    );
  $form['calendar_full_day_header'] = array(
    '#title' => t('Full day name size'),
    '#default_value' => variable_get('calendar_full_day_header_'. $view->name, 3),
    '#type' => 'select',
    '#options' => array(1 => t('First letter of name'), 2 => t('First two letters of name'), 3 => t('Abbreviated name'), 99 => t('Full name')),
    '#description' => t('The way day of week names should be displayed in a full size calendar.'),
    );
  $form['calendar_weekno'] = array(
    '#title' => t('Show week numbers'),
    '#type' => 'select',
    '#options' => array(0 => t('No'), 1 => t('Yes')),
    '#default_value' => variable_get('calendar_weekno_'. $view->name, 1),
    '#description' => t('Whether or not to display a clickable week number link on the left side of each calendar week.')
    );    
  $form['calendar_popup'] = array(
    '#title' => t('Popup date selector'),
    '#type' => 'select',
    '#options' => array(0 => t('No'), 1 => t('Yes')),
    '#default_value' => variable_get('calendar_popup_'. $view->name, module_exists('date_popup')),
    '#description' => t('Whether or not to display a popup date selector to change the calendar period. (Only works when the Date Popup module is enabled.)')
    );
  $form['calendar_limit'] = array(
    '#title' => t('Maximum items'),
    '#type' => 'select',
    '#options' => array(0 => t('Unlimited'), 3 => t('3 items'), 5 => t('5 items'), 10 => t('10 items')),
    '#default_value' => variable_get('calendar_limit_'. $view->name, 5),
    '#description' => t('Maximum number of items to show in calendar cells, used to keep the calendar from expanding to a huge size when there are lots of items in one day. '),
    );
  $form['calendar_limit_behavior'] = array(
    '#title' => t('Too many items'),
    '#type' => $calendar_type != 'day' ? 'select' : 'hidden',
    '#options' => array('more' => t("Show maximum, add 'more' link"), 'hide' => t('Hide all, add link to day')),
    '#default_value' => variable_get('calendar_limit_behavior_'. $view->name, 'more'),
    '#description' => t('Behavior when there are more than the above number of items in a single day. When there more items than this limit, a link to the day view will be displayed.'),
    );
  $form['truncate_length'] = array(
    '#title' => t('Truncate length'),
    '#type' => 'textfield',
    '#default_value' => variable_get('calendar_truncate_length_'. $view->name, ''),
    '#description' => t("Truncate size for titles in month and week view so things fit better into the calendar cell. For instance, change the title from 'Very Very Very Long Name' to something like 'Very Very...'."),
    '#maxlength' => 4,
    '#size' => 4,
    );

  $display_options = array('calendar' => t('Calendar'), 'table' => t('Table'), 'teasers' => t('Teasers'), 'nodes' => t('Full Nodes'), 'list' => t('List'), '' => t('None'));
  $display_format = variable_get('calendar_display_format_'. $view->name, array('year' => 'calendar', 'month' => 'calendar', 'week' => 'calendar', 'day' => 'calendar', 'block' => 'calendar'));
  $form['display'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display options'),
    '#description' => t('Choose the way the calendar entries should be displayed. Selecting \'None\' will hide links to that option.'),
    );
  $form['display']['year'] = array(
    '#title' => t('Year display'),
    '#default_value' => $display_format['year'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['display']['month'] = array(
    '#title' => t('Month display'),
    '#default_value' => $display_format['month'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['display']['week'] = array(
    '#title' => t('Week display'),
    '#default_value' => $display_format['week'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['display']['day'] = array(
    '#title' => t('Day display'),
    '#default_value' => $display_format['day'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['display']['block'] = array(
    '#title' => t('Block display'),
    '#default_value' => $display_format['block'],
    '#type' => 'select',
    '#options' => $display_options,
  );
  $form['calendar_year_range'] = array(
    '#title' => t('Date year range'),
    '#type' => 'textfield',
    '#default_value' => variable_get('calendar_year_range_'. $view->name, '-3:+3'),
    '#description' => t("Set the allowable minimum and maximum year range for this view, either a -X:+X offset from the current year, like '-3:+3' or an absolute minimum and maximum year, like '2005:2010'.  When the argument is set to a date outside the range, the page will be returned as 'Page not found (404)'."),
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
    '#value' => t('Submit'),
    );
  return $form;
}

/**
 * Setup Calendar legend parameters.
 */
function _calendar_legend_form($view_name) {
  $view = views_get_view($view_name);
  $form = array();
  $form['markup'] = array(
    '#type' => 'markup',
    '#value' => _calendar_settings_header($view) .  t('<h3>Content Type</h3><p>Set a hex color value (like #ffffff) to use in the calendar legend for each content type. Types with empty values will have no stripe in the calendar and will not be added to the legend.</p>'),
    ); 
  //TODO Add in methods other than content types for setting legend colors.
  $method =  variable_get('calendar_legend_method_'. $view_name, 'types');
  
  // TODO Move the embedded styles other than the color into a stylesheet.
  $form['calendar_colorpicker'] = array(
      '#type' => 'calendar_colorpicker',
      '#title' => t('Legend colors'),
      '#prefix' => '<div style="float:left">',
      '#suffix' => '</div>',
    );
  $form['color'] = array('#tree' => TRUE);
  $colors = variable_get('calendar_color_'. $view_name, $default_colors); 
  
  switch ($method) {
    case 'types':
      $color_types = node_get_types('names');
      break;
  }
  foreach ($color_types as $key => $name) {
    $form['color'][$key] = array(
      '#title' => $name,
      '#type' => 'calendar_colorfield',
      '#default_value' => isset($colors[$key]) ? $colors[$key] : '#ffffff',
      '#calendar_colorpicker' => 'calendar_colorpicker',
      '#prefix' => '<div style="float:left;margin-right:10px">',
      '#suffix' => '</div>',
    );
  }
  $form['view_name'] = array(
    '#type' => 'hidden',
    '#value' => $view->name,
    );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#prefix' => '<div style="clear:both;">',
    '#suffix' => '</div>',
    );
  return $form;
}

function _calendar_setup_form_validate($form_id, $form_values) {
  if (!preg_match('@\-[0-9]*:[\+|\-][0-9]*@', $form_values['calendar_year_range']) 
    && !preg_match('@[0-9]{4}:[0-9]{4}@', $form_values['calendar_year_range'])) {
    form_set_error('calendar_year_range', t('Date year range must be in the format -9:+9 or 2005:2010.'));
  }
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
    case 'calendar_year_range':
        variable_set('calendar_year_range_'. $view_name,  $value);
        break;
    case 'calendar_limit':
        variable_set('calendar_limit_'. $view_name,  $value);
        break;
    case 'calendar_limit_behavior':
        variable_set('calendar_limit_behavior_'. $view_name,  $value);
        break;
    case 'calendar_time_format':
        variable_set('calendar_time_format_'. $view_name,  $value);
        break;
    case 'calendar_day_header':
        variable_set('calendar_day_header_'. $view_name,  $value);
        break;
    case 'calendar_full_day_header':
        variable_set('calendar_full_day_header_'. $view_name,  $value);
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
  variable_set('calendar_weekno_'. $view_name, check_plain($form_values['calendar_weekno']));
  variable_set('calendar_popup_'. $view_name, check_plain($form_values['calendar_popup']));
  variable_set('calendar_truncate_length_'. $view_name, check_plain($form_values['calendar_truncate_length']));
  variable_set('calendar_display_format_'. $view_name, $display_format);
  drupal_set_message(t('Calendar settings have been updated.'));
}

/**
 * Save requested values.
 */
function _calendar_legend_form_submit($form_id, $form_values) {
  $view_name = $form_values['view_name'];
  variable_set('calendar_color_'. $view_name, $form_values['color']);
  drupal_set_message(t('Calendar legend settings have been updated.'));
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
      'theme' => 'calendar_views_calendar',
      'summary_theme' => 'calendar_views_summary',
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
  $view->access = array();
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
  $view->sort = array();
  $view->argument = array(
    array(
      'type' => 'calendar_year',
      'argdefault' => '2',
      'title' => '%1',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array(
      'type' => 'calendar_month',
      'argdefault' => '2',
      'title' => '%2',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array(
      'type' => 'calendar_day',
      'argdefault' => '2',
      'title' => '%3',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
  );
  $view->field = array(
    array(
      'tablename' => 'node',
      'field' => 'title',
      'label' => 'Title:',
      'handler' => 'views_handler_field_nodelink',
      'options' => 'link',
    ),
  );
  $view->filter = array(
    array(
      'tablename' => 'node',
      'field' => 'status',
      'operator' => '=',
      'options' => '',
      'value' => '1',
    ),
  );
  $view->exposed_filter = array(
  );
  $view->requires = array(node);
  $views[$view->name] = $view;
  return $views;
}
