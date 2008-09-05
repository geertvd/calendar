<?php
//$Id$
/**
 * @file
 * All the code used while processing a calendar is stored in this file
 * and is included only when needed.
 */
function _calendar_views_query_alter(&$query, &$view) {
  // make sure block views default to the current month
  // and make sure day view is not selected
  $now = date_now();
  $view->real_args = $view->args;
  $view->real_url = calendar_real_url($view, $view->args);
  $display_formats = calendar_get_formats($view);
  $calendar_fields = calendar_fields();
  foreach ($view->field as $field) {
    if (in_array($field['field'], array_keys($calendar_fields))) {
      $calendar_field = $calendar_fields[$field['field']];
      if (!empty($calendar_field['delta_field'])) {
        $query->add_field($calendar_field['delta_field'], $calendar_field['table']);
      }
    }
  }
  
  // Embedded calendar views can have additional arguments.
  if ($view->build_type == 'block' || $view->build_type == 'embed') {
    $block_identifier = isset($view->block_identifier) ? $view->block_identifier : 'mini';
    $pos = calendar_arg_positions($view);
    
    // If we don't have any calendar arguments, look in the block_identifier for them.
    if ($pos[0] >= count($view->args) && !empty($_GET[$block_identifier])) {
      // Add the arguments from the block_identifier query param.
      $view->real_args = $view->args = array_merge($view->args, explode('/', str_replace($view->real_url .'/', '', $_GET[$block_identifier])));
      // Filter the query based on the newly acquired arguments.
      foreach ($view->argument as $delta => $argument) {
        // Special handling for OG gid argument.
        // Find a default value for the gid when used in a block.
        if ($argument['type'] == 'gid') {
          $groupnodes = calendar_og_groups($view);
          $view->args[$delta] = $groupnodes[0];
          $query->ensure_table('og_ancestry');
          $query->add_where("og_ancestry.group_nid IN (%d)", implode(',', $groupnodes));
        }
        if (!empty($view->args[$delta]) && $view->args[$delta] != CALENDAR_EMPTY_ARG) {
          if ($argument['type'] == 'calendar_year') {
            calendar_filter_year($query, $view->args[$delta]);
          }
          elseif ($argument['type'] == 'calendar_month') {
            calendar_filter_month($query, $view->args[$delta]);
          }
          elseif ($argument['type'] == 'calendar_day') {
            calendar_filter_day($query, $view->args[$delta]);
          }
        }
      }
    }
  }

  // If no arguments are provided, default to current month view.
  if (empty($view->args) || !calendar_is_calendar_arg($view)) {
  	$query->calendar_type = 'month';
    foreach ($view->argument as $delta => $argument) {
      if ($argument['type'] == 'calendar_year' && !$view->args[$delta]) {
        $view->args[$delta] = date_format($now, 'Y');
        calendar_filter_year($query, date_format($now, 'Y'));
      }
      elseif ($argument['type'] == 'calendar_month' && !$view->args[$delta]) {
        $view->args[$delta] = date_format($now, 'm');
        calendar_filter_month($query, date_format($now, 'm'));
      }
      elseif ($argument['type'] == 'calendar_day' && !$view->args[$delta]) {
        $view->args[$delta] = CALENDAR_EMPTY_ARG;
      }
      else {
        $view->args[$delta] = $view->real_args[$delta];
      }
    }
    // Default arguments are real arguments, too.
    $view->real_args = $view->args;
  }
  foreach ($view->argument as $delta => $argument) {
    if (in_array($argument['type'], calendar_args())) {
      // make sure 'display all values' is selected for the calendar arguments
      // summary views are meaningless and create errors in this context
      $view->argument[$delta]['argdefault'] = 2;

      // Pad any unused values in the view arguments with
      // CALENDAR_EMPTY_ARG to indicate all values.
      if (empty($view->args[$delta])) {
      	$view->args[$delta] = CALENDAR_EMPTY_ARG;
      }
    }

    // Calendar_week and Calendar_month can swap positions as the second arg
    // in the url. Do some work here to make sure we know which is which and
    // swap view data to match it. The difference between a calendar_month
    // arg and a calendar_week arg is the preceding 'W'
    if ($argument['type'] == 'calendar_week' || $argument['type'] == 'calendar_month') {

      if (strstr($view->args[$delta], 'W')) {
      	calendar_filter_week($query, $view->args[$delta]);
        $view->argument[$delta]['type'] = 'calendar_week';
        $view->argument[$delta]['id'] = 'calendar_week';
        $view->argument[$delta + 1]['type'] = 'calendar_day';
        $view->argument[$delta + 1]['id'] = 'calendar_day';

        // Make sure that there is no day set for the week view.
        $view->args[$delta + 1] = CALENDAR_EMPTY_ARG;
      }
      // if this is not a week argument and view was created with a
      // week argument, change it back
      elseif (($view->build_type == 'page' || $view->build_type == 'embed')
        && $view->argument[$delta]['type'] == 'calendar_week') {
        calendar_filter_month($query, $view->args[$delta]);
        $view->argument[$delta]['type'] = 'calendar_month';
        $view->argument[$delta]['id'] = 'calendar_month';
        $view->argument[$delta + 1]['type'] = 'calendar_day';
        $view->argument[$delta + 1]['id'] = 'calendar_day';
      }
    }
  }

  // Make sure the calendar query gets inserted. May not have finished yet
  // on views like year or year/month.
  if (!$query->calendar_finished) {
    calendar_build_filter($query, $view);
  }
  $view->calendar_type = $query->calendar_type;
  $view->year = $query->year;
  $view->month = $query->month;
  $view->day = $query->day;
  $view->week = $query->week;
  $view->min_date = $query->min_date;
  $view->max_date = $query->max_date;
  $view->min_date_date = date_format($view->min_date, DATE_FORMAT_DATE);
  $view->max_date_date = date_format($view->max_date, DATE_FORMAT_DATE);
  
  // Identify the kind of display we're using for this view.
  // Check first for 'view' in url to get displays set by the switch
  // block, then for 'calendar_display_format' variable to get displays
  // set in the setup, default to normal calendar display.
  
  if ($view->build_type == 'block') {
    $view->calendar_display = $display_formats['block'];
  }
  elseif (isset($_GET['view']) && $view->build_type == 'page') {
  	$view->calendar_display = !empty($_GET['view']) ? check_plain($_GET['view']) : 'calendar';
  }
  elseif ($view->build_type == 'page' || $view->build_type == 'embed') {
  	$display_formats = calendar_get_formats($view);
  	$view->calendar_display = $display_formats[$view->calendar_type];
  }
  
  // Global used to turn on the "Switch calendar" block.
  if ($view->build_type == 'page') {
    $GLOBALS['calendar_is_calendar'] = TRUE;
  }

  return;
}

/**
 * Build calendar
 *
 * @param unknown_type $view
 * @param unknown_type $items
 * @param unknown_type $params
 * @return themed table
 */
function calendar_build_calendar($view, $items, $params) {
  // Remove nodes outside the selected date range.
  $values = array();
  foreach ($items as $delta => $item) {
    if (empty($item->calendar_start_date) || empty($item->calendar_end_date)) {
      continue;
    }
    $item_start = date_format($item->calendar_start_date, DATE_FORMAT_DATE);
    $item_end = date_format($item->calendar_end_date, DATE_FORMAT_DATE);
    if (($item_start >= $view->min_date_date && $item_start <= $view->max_date_date)
     || ($item_end >= $view->min_date_date && $item_end <= $view->max_date_date)) {
      $values[$item_start][$item_start][] = $item;
    }
  }
  $items = $values;
  ksort($items);

  $rows = array();
  $curday = drupal_clone($view->min_date);
  date_timezone_set($curday, timezone_open(date_default_timezone_name()));
  date_time_set($curday, 0, 0, 0);
  switch ($view->calendar_type) {
    case 'year':
      $rows = array();
      for ($i = 1; $i <= 12; $i++) {
        $rows[$i] = calendar_build_month($curday, $view, $items, $params);
      }
      break;

    case 'month':
      $rows = calendar_build_month($curday, $view, $items, $params);
      break;

    case 'day':
      $rows = calendar_build_day($curday, $view, $items, $params);
      break;

    case 'week':
      $rows = calendar_build_week($curday, $view, $items, $params);

      // Merge the day names in as the first row.
      if (!empty($view->mini)) {
        $len = variable_get('calendar_day_header_'. $view->name, 1);
      }
      else {
        $len = variable_get('calendar_full_day_header_'. $view->name, 3);
      }
      $rows = array_merge(array(calendar_week_header($view->mini, $params['with_weekno'], $len)), $rows);
      break;
  }

  $output  = theme('calendar_links', $view, $view->build_type != 'block');
  $output .= theme('calendar_nav', $view);
  $header = in_array($view->calendar_type, array('month', 'week')) ? array_shift($rows) : array();
  $output .= theme('calendar_'. $view->calendar_type, $view, $header, $rows);
  return $output;
}

/**
 * Build one month.
 */
function calendar_build_month(&$curday, $view, $items, $params) {
  $month = date_format($curday, 'n');
  date_modify($curday, '-' . strval(date_format($curday, 'j')-1) . ' days');

  $rows = array();
  do {
    $rows = array_merge($rows, calendar_build_week($curday, $view, $items, $params, TRUE));
    $curday_date = date_format($curday, DATE_FORMAT_DATE);
    $curday_month = date_format($curday, 'n');
  } while ($curday_month == $month && $curday_date <= $view->max_date_date);

  // Merge the day names in as the first row.
  if (!empty($view->mini)) {
    $len = variable_get('calendar_day_header_'. $view->name, 1);
  }
  else {
    $len = variable_get('calendar_full_day_header_'. $view->name, 3);
  }
  $rows = array_merge(array(calendar_week_header($view->mini, $params['with_weekno'], $len)), $rows);
  return $rows;
}

/**
 * Build one week row.
 */
function calendar_build_week(&$curday, $view, $items, $params, $check_month = FALSE) {
  $curday_date = date_format($curday, DATE_FORMAT_DATE);
  $weekdays = calendar_untranslated_days($results, $view);
  $today = date_format(date_now(date_default_timezone_name()), DATE_FORMAT_DATE);
  $month = date_format($curday, 'n');
  $week = date_week($curday_date);
  $first_day = variable_get('date_first_day', 0);

  // move backwards to the first day of the week
  $day_wday = date_format($curday, 'w');
  date_modify($curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');
  $curday_date = date_format($curday, DATE_FORMAT_DATE);
    
  // If we're displaying the week number, add it as the
  // first cell in the week.
  $display_formats = calendar_get_formats($view);
  if ($params['with_weekno'] && $view->calendar_type != 'day') {
    if (!empty($display_formats['week'])) {
      if ($view->build_type == 'embed') {
        $url = $params['url'];
        $block_identifier = isset($view->block_identifier) ? $view->block_identifier : 'mini';
        $query_string = (!empty($params['append']) ? $params['append'] .'&' : '') . $block_identifier .'='. $params['url'] .'/'. $view->year .'/W'. $week;
      }
      else {
        $url = $params['url'] .'/'. $view->year .'/W'. $week;
        $query_string = $params['append'];
      }
      $weekno = l($week, $url, NULL, $query_string);
    }
    else { 
      // Do not link week numbers, if Week views are disabled.
      $weekno = $week;
    }
    $rows[$week][] = array(
      'data' => $weekno,
      'class' => 'week',
      );
  }
  for ($i = 0; $i < 7; $i++) {
    $curday_date = date_format($curday, DATE_FORMAT_DATE);
    if ($check_month && ($curday_date < $view->min_date_date || $curday_date > $view->max_date_date || date_format($curday, 'n') != $month)) {
      $class = strtolower($weekdays[$i] .
      ($view->mini ? ' mini' : ''));
      $content = theme('calendar_empty_day');
    }
    else {
      $content = calendar_build_day($curday, $view, $items, $params);
      $class = strtolower($weekdays[$i] .
      ($curday_date == $today ? ' today' : '') .
      ($curday_date < $today ? ' past' : '') .
      ($curday_date > $today ? ' future' : '') .
      ($view->mini ? ' mini' : ''));
    }

    $rows[$week][] = array(
      'data' => $view->mini ? $content : '<div class="inner">'. $content .'</div>',
      'class' => $class, 'id' => $view->name . '-' . $curday_date);
    date_modify($curday, '+1 day');
  }

  return $rows;
}

/**
 * Build the contents of a single day for the $rows results.
 */
function calendar_build_day($curday, $view, $items, $params) {
  $curday_date = date_format($curday, DATE_FORMAT_DATE);
  $inner = '';
  $selected = FALSE;
  foreach ($items as $date => $day) {
    if ($date == $curday_date) {
      $selected = TRUE;
      ksort($day);
      foreach ($day as $time) {
        foreach ($time as $item) {
          if (!$view->mini) {
            $theme = isset($item->calendar_node_theme) ? $item->calendar_node_theme : 'calendar_node_'. $view->calendar_type;
            $inner .= theme($theme, $item, $view);
          }
        }
      }
    }
  }
  if (empty($inner)) {
    $inner = theme('calendar_empty_day');
  }
  $content = theme('calendar_date_box', $curday_date, $view, $items, $params, $selected) . $inner;
  return $content;
}

/**
 * Formats the weekday information into table header format
 *
 * @ingroup event_support
 * @return array with weekday table header data
 */
function calendar_week_header($mini = FALSE, $with_week = TRUE, $len = 1) {
  // create week header
  $untranslated_days = calendar_untranslated_days();
  if ($len == 99) {
    $translated_days = date_week_days_ordered(date_week_days(TRUE));
  }
  else {
    $translated_days = date_week_days_ordered(date_week_days_abbr(TRUE));
  }

  if ($with_week) {
    $row[] = array('header' => TRUE, 'class' => "days week", 'data' => '&nbsp;');
  }
  foreach ($untranslated_days as $delta => $day) {
    $label = $mini ? drupal_substr($translated_days[$delta], 0 , $len) : $translated_days[$delta];
    $row[] = array('header' => TRUE, 'class' => "days ". $day, 'data' => $label);
  }
  return $row;
}

/**
 * Array of untranslated day name abbreviations, forced to lowercase
 * and ordered appropriately for the site setting for the first day of week.
 *
 * The untranslated day abbreviation is used in css classes.
 */
function calendar_untranslated_days() {
  $untranslated_days = date_week_days_ordered(date_week_days_untranslated());
  foreach ($untranslated_days as $delta => $day) {
    $untranslated_days[$delta] = strtolower(substr($day, 0, 3));
  }
  return $untranslated_days;
}

/**
 * Compile the filter query for this view.
 *
 * Create date objects for the minimum and maximum possible dates for this
 * view and store them in the query (and ultimately in the view),
 * then create the query needed to find dates in that range.
 *
 * @param object $query
 * @param object $view
 */
function calendar_build_filter(&$query, &$view) {
  $now = date_now();

  if ($query->calendar_type == 'week' && calendar_part_is_valid($query->week, 'week')) {
    $range = date_week_range($query->week, $query->year);
    $date = $range[0];
    $max_date = $range[1];
  } else {
    $month = calendar_part_is_valid($query->month, 'month') ? $query->month : 1;
    $day   = calendar_part_is_valid($query->day, 'day') ? $query->day : 1;
    $year  = calendar_part_is_valid($query->year, 'year') ? $query->year : date_format($now, 'Y');
    $date  = date_create($year .'-'. date_pad($month) .'-'. date_pad($day) .' 00:00:00', date_default_timezone());
    $max_date = drupal_clone($date);
    date_modify($max_date, '+1 '. $query->calendar_type);
    date_modify($max_date, '-1 second');
  }
  $query->min_date = $date;
  $query->max_date = $max_date;

  // find all datetime fields in this view and add filters for them to the query
  $queries = array();
  foreach ($view->field as $delta => $field) {
    $query_strings = calendar_build_field_query($query, $field);
    if (!empty($query_strings)) $queries = array_merge($queries, $query_strings);
  }
  // bring the node type into the query so we can use it in the theme
  $query->add_field('type', 'node');
  if ($queries) $query->add_where(implode(" OR ", $queries));

  return;
}

/**
 * Build a filtering query for an individual date field
 *
 * @param object $query - the query object
 * @param array $field - the view field array
 */
function calendar_build_field_query(&$query, $field) {
  require_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
	$queries = array();
	$fields = calendar_fields();
  $field_name = $field['field'];
  $this_field = $fields[$field_name];
  $view_fields[] = $field_name;
  $field_type   = strstr($this_field['type'], 'string') ? DATE_ISO : DATE_UNIX;

  // Create minimum and maximum comparison dates in DATETIME format.
  $min_compare = drupal_clone($query->min_date);
  $max_compare = drupal_clone($query->max_date);
  $min = date_format($min_compare, DATE_FORMAT_DATETIME);
  $max = date_format($max_compare, DATE_FORMAT_DATETIME);

  if (array_key_exists($field_name, $fields)) {
    $query->ensure_table($this_field['table'], $this_field['table']);
    $tz_handling  = $this_field['tz_handling'];
    $date_handler = new date_sql_handler();
    $date_handler->construct($field_type);
          
    switch ($tz_handling) {
      case 'date' :
        $date_handler->db_timezone = 'UTC';
        $date_handler->local_timezone_field = $fields['tz_field'];
        $date_handler->local_offset_field = $fields['offset_field'];
        break;
      // 'None' doesn't actually use UTC, but we've set the db timezone
      // to UTC so setting the local timezone to UTC will accomplish
      // what we want to accomplish - do no timezone conversion.
      case 'none':
      case 'utc':
        $date_handler->db_timezone = 'UTC';
        $date_handler->local_timezone = 'UTC';
        break;
      default :
        $date_handler->db_timezone = 'UTC';
        $date_handler->local_timezone = date_default_timezone_name();
        break;
    }

    // Figure out where this field is in the query's field array
    // so we know which query field to adjust.
    foreach ($query->fields as $delta => $query_field) {
      if (strstr($query_field, $this_field['fullname'] .' AS')) {
        $field_delta = $delta;
      }
    }

    if ($this_field['fromto'] && $this_field['fromto'][0] != $this_field['fromto'][1]) {
      // Format dates with from and to times into a single value that is made
      // up of the two local DATETIME values separated with a pipe (|).
      $queries[] = "(". $date_handler->sql_where_date('DATE', $this_field['fromto'][1], '>=', $min) .
        " AND ". $date_handler->sql_where_date('DATE', $this_field['fromto'][0], '<=', $max) .")";
    }
    else {
      $queries[] = "(". $date_handler->sql_where_date('DATE', $this_field['fullname'], '>=', $min) .
        ' AND '. $date_handler->sql_where_date('DATE', $this_field['fullname'], '<=', $max) .")";
    }
  }
  return $queries;
}

/**
 * Take the array of items and alter it to an array of
 * calendar nodes that the theme can handle.
 *
 * Iterate through each datefield in the view and each item
 * returned by the query, and create pseudo date nodes.
 *
 * If there is more than one date field in the node, this will create
 * multiple nodes, one each with the right calendar date for that
 * field's value. If a field value has a date range that covers more than
 * one day, separate nodes will be created for each day in the field's
 * day range, limited to the minimum and maximum dates for the view.
 *
 * When we finish, we will have a distinct node for each distinct day
 * and date field.
 */
function calendar_build_nodes(&$view, &$items) {
  if (empty($view->min_date) || empty($view->max_date)) {
    return $items;
  }
  
  // Midnights are determined based on the same timezone as the View uses
  $display_timezone = date_timezone_get($view->min_date);
  $display_timezone_name = timezone_name_get($display_timezone);
  
  // Translate the view min and max dates to UTC values
  // so we can compare UTC dates to the view range.
  $min_utc = drupal_clone($view->min_date);
  date_timezone_set($min_utc, timezone_open('UTC'));
  $max_utc = drupal_clone($view->max_date);
  date_timezone_set($max_utc, timezone_open('UTC'));
  $min_zone_string = array(); // Will cache $min_utc-strings in various timezones 
  $max_zone_string = array();

  $view->nodes_per_page = 0;
  $type_names = node_get_types('names');

  $fields = calendar_fields();
  foreach($fields as $field) {
    $datefields[] = $field['query_name'];
  }
  $view_fields      = _views_get_fields();
  $field_names = (array) array_keys($fields);
  $nodes = array();
  $i = 0;

  // explode out field and format info from the view
  foreach ($view->field as $delta => $data) {
    if ($fields[$data['id']]['visible'] !== FALSE && array_key_exists($data['field'], $fields)) {
      if (in_array($data['field'], $field_names)) {
        $field        = $fields[$data['field']];
        $field_type   = strstr($field['type'], 'string') ? 'string' : 'timestamp';
        $tz_handling  = $field['tz_handling'];
        $label        = $field['label'];
        $fromto       = $field['fromto'];
        $fromto_alias = array(str_replace('.', '_', $fromto[0]), str_replace('.', '_', $fromto[1]));
        $tz_alias     = str_replace('.', '_', $field['timezone_field']);
        
        if (strstr($field['type'], 'cck')) {
          $format = date_formatter_format($data['options'], $field['field_name']);
        }
        else {
          switch ($data['handler']) {
            case 'views_handler_field_date_large':
              $format = variable_get('date_format_long',  'l, F j, Y - H:i');
              break;
            case 'views_handler_field_date':
              $format = variable_get('date_format_medium',  'D, m/d/Y - H:i');
              break;
            case 'views_handler_field_date_custom':
              $format = $data['options'];
              break;
            case 'views_handler_field_since':
            case 'views_handler_field_date_small':
            default:
              $format = variable_get('date_format_short', 'm/d/Y - H:i');
              break;
          }
        }

        foreach ($items as $pos => $item) {
          if (isset($item->{$field['query_name']})) {
            
            // Create from and to date values for each item, adjusted to
            // the correct timezone.
            $values = array(
              $item->$fromto_alias[0], 
              isset($item->$fromto_alias[1]) ? $item->$fromto_alias[1] : $item->$fromto_alias[0],
              );
            
            $db_tz   = date_get_timezone_db($tz_handling, isset($item->$tz_alias) ? $item->$tz_alias : $display_timezone_name);
            $to_zone = date_get_timezone($tz_handling, isset($item->$tz_alias) ? $item->$tz_alias : $display_timezone_name);
        
            // NOTE: Now $display_timezone determines how $item is split
            // into one entry per day, while $to_zone determines how date is displayed.
            // Maybe someone wants to the date fields's timezone for the day splitting,
            // then use: $display_timezone_name = $to_zone;
            $values_display = array();
            
            // Start date
            $date = date_make_date($values[0], $db_tz, $field['sql_type']);
            if ($db_tz != $to_zone) {
              date_timezone_set($date, timezone_open($to_zone));
            }
            $values[0] = date_format($date, DATE_FORMAT_DATETIME);
            
            if ($display_timezone_name != $to_zone) {
              date_timezone_set($date, $display_timezone);
              $values_display[0] = date_format($date, DATE_FORMAT_DATETIME);
            } 
            else {
              $values_display[0] = $values[0];
            }
            
            // End date
            $date = date_make_date($values[1], $db_tz, $field['sql_type']);
            if ($db_tz != $to_zone) {
              date_timezone_set($date, timezone_open($to_zone));
            }
            $values[1] = date_format($date, DATE_FORMAT_DATETIME);
            if ($display_timezone_name != $to_zone) {
              date_timezone_set($date, $display_timezone);
              $values_display[1] = date_format($date, DATE_FORMAT_DATETIME);
            } 
            else {
              $values_display[1] = $values[1];
            }
            
            // Now $values contain start and end date of a node,
            // expressed as strings in the display (local) timezone.
            // $values_utc does the same in UTC timezone.

            // Get calendar min and max day (not time) as strings in the
            // $display_timezone. Cache in $min_zone_string and $max_zone_string,
            // since many items or fields typically use the samee timezone.
            if (!isset($min_zone_string[$display_timezone_name])) {
              $date = drupal_clone($view->min_date);
              date_timezone_set($date, $display_timezone);
              $min_zone_string[$display_timezone_name] = date_format($date, DATE_FORMAT_DATE);
              $date = drupal_clone($view->max_date);
              date_timezone_set($date, $display_timezone);
              $max_zone_string[$display_timezone_name] = date_format($date, DATE_FORMAT_DATE);
            }
            // Create a node for each date within the field's date range,
            // limited to the view's date range (regarding only day, not time).
            $now = max($min_zone_string[$display_timezone_name], substr($values_display[0], 0, 10));
            $to  = min($max_zone_string[$display_timezone_name], substr($values_display[1], 0, 10));
            $next = date_make_date($now, $display_timezone);
            if ($display_timezone_name != $to_zone) {
              // Make $start and $end (derived from $node) use the timezone $to_zone, just as $values[..] do
              date_timezone_set($next, timezone_open($to_zone));
            }
            
            if (empty($to)) {
              $to = $now;
            }
            // $now and $next are midnight (in display timezone) on the first day where node will occur.
            // $to is midnight on the last day where node will occur.
            // All three were limited by the min-max date range of the view.
            while ($now <= $to) {
              $node = drupal_clone($item);
              // Make sure the pseudo node has the same properties a
              // regular node would have.
              if (isset($node->node_title) && !isset($node->title)) {
                $node->title = $node->node_title;
              }
              if (isset($node->node_type) && !isset($node->type)) {
                $node->type = $node->node_type;
              }

              $node->label = $label;
              $node->format = $format;
              $node->format_time = variable_get('calendar_time_format_'. $view->name, 'H:i');
              $node->url = calendar_get_node_link($node);
              
              // Convert the table.field format in the fromto fields
              // to the table_field format used by the Views formatters.
              $node->{str_replace('.', '_', $fromto[0])} = $values[0];
              $node->{str_replace('.', '_', $fromto[1])} = $values[1];

              // Flag which datefield this node is using, in case
              // there are multiple date fields in the view.
              $node->datefield = $field['query_name'];

              // If there are other datefields in the View, get rid
              // of them in this pseudo node. There should only be one
              // date in each calendar node.
              foreach ($node as $key => $val) {
                if ($key != $field['query_name'] && in_array($key, $datefields)) {
                  unset($node->$key);
                  foreach ($fields as $other_fields) {
                    // If the unused date has other fields, unset them, too.
                    if ($other_fields['query_name'] == $key) {
                      foreach ($other_fields['related_fields'] as $related_field) {
                        $key2 = str_replace('.', '_', $related_field);
                        unset($node->$key2);
                      }
                    }
                  }
                }
              }

              // Get start and end of current day
              $start = date_format($next, DATE_FORMAT_DATETIME);
              date_modify($next, '+1 day');
              date_modify($next, '-1 second');
              $end = date_format($next, DATE_FORMAT_DATETIME);

              // Get intersection of current day and the node value's duration (as strings in $to_zone timezone)
              $node->calendar_start = $values[0] < $start ? $start : $values[0];
              $node->calendar_end = !empty($values[1]) ? ($values[1] > $end ? $end : $values[1]) : $node->calendar_start;

              // Make date objects
              $node->calendar_start_date = date_create($node->calendar_start, timezone_open($to_zone));
              $node->calendar_end_date = date_create($node->calendar_end, timezone_open($to_zone));
              if ($tz_handling != 'utc' && $tz_handling != 'none') {
                date_timezone_set($node->calendar_start_date, date_default_timezone());
                date_timezone_set($node->calendar_end_date, date_default_timezone());
              } 
              else {
                date_timezone_set($node->calendar_start_date, timezone_open('UTC'));
                date_timezone_set($node->calendar_end_date, timezone_open('UTC'));
              }
              // Change string timezones into 
              // calendar_start and calendar_end are UTC dates as formatted strings
              $node->calendar_start = date_format($node->calendar_start_date, DATE_FORMAT_DATETIME);
              $node->calendar_end = date_format($node->calendar_end_date, DATE_FORMAT_DATETIME);
              
              unset($node->calendar_fields);
              if (isset($node) && (empty($node->calendar_start))) {
                // if no date for the node and no date in the item
                // there is no way to display it on the calendar
                unset($node);
              }
              else {
                $delta = !empty($field['delta_field']) && !empty($item->{$field['delta_field']}) ? $item->{$field['delta_field']} : 0;
                $real_field = $field['field_name'];
                if (substr($field['type'], 0, 3) == 'cck') {
                  $real_field = str_replace(array('_value2', '_value'), '', $field['field_name']);
                }
                $id = 'calendar:'. $item->nid .':'. $real_field .':'. $delta .':'. $pos;
                $node->date_id = $id;
                if ($view->build_type == 'page' && $view->calendar_type != 'year') {
                  calendar_node_stripe($view, $node, $field['query_name'], $field['query_name']);
                }
                $nodes[] = $node;
                unset($node);
              }
              date_modify($next, '+1 second');
              $now = date_format($next, DATE_FORMAT_DATE);
            }
          }
        }
      }
    }
  }
  return $nodes;
}

function calendar_get_paths($view) {
  $path = array();
  $prev_date = drupal_clone($view->min_date);
  date_modify($prev_date, '-1 '. $view->calendar_type);
  $next_date = drupal_clone($view->min_date);
  date_modify($next_date, '+1 '. $view->calendar_type);
  
  $path = calendar_get_url($view, $view->args, TRUE);
  
  // reverse through the path, creating a $nextpath and $prevpath arrays
  $formats = array('day' => 'j', 'month' => 'm', 'year' => 'Y', 'week' => 'W');
  for ($x = count($path); $x >= 0; $x--) {
    $type = $path[$x]['type'];
    if (in_array($type, array_keys($formats)) && $path[$x]['path'] != CALENDAR_EMPTY_ARG) {
      if ($type != 'week') {
        $nextpath[$x] = $type == 'year' && isset($next_year) ? $next_year : date_format($next_date, $formats[$type]);
        $prevpath[$x] = $type == 'year' && isset($prev_year) ? $prev_year : date_format($prev_date, $formats[$type]);
      }
      else {
        if (date_format($prev_date, 'Y') < $view->year) {
          $prev_week = calendar_max_weeks(date_format($prev_date, 'Y'));
        }
        else {
          $prev_week = $view->week - 1;
        }
        if (date_format($next_date, 'Y') > $view->year) {
          $next_week = 1;
        }
        else {
          $next_week = $view->week + 1;
        }
        $nextpath[$x] = 'W'. $next_week;
        $prevpath[$x] = 'W'. $prev_week;
      }
    }
    // Non-date path elements just get passed through.
    elseif (!empty($path[$x]['path'])) {
      $nextpath[$x] = $path[$x]['path'];
      $prevpath[$x] = $path[$x]['path'];
    }
  }
  
  return array($prevpath, $nextpath);
}