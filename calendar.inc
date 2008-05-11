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

  if ($view->build_type == 'block') {
    $query->calendar_type = 'month';
    $view->calendar_display = $display_formats['block'];
    if (empty($view->args)) {
      $view->args = explode('/', str_replace($view->url .'/', '', $_GET['mini']));
    }
    foreach ($view->argument as $delta => $argument) {
      // Special handling for OG gid argument.
      // Find a default value for the gid when used in a block.
      if ($argument['type'] == 'gid') {
        $groupnodes = calendar_og_groups($view);
        $view->args[$delta] = $groupnodes[0];
        $query->ensure_table('og_ancestry');
        $query->add_where("og_ancestry.group_nid IN (%d)", implode(',', $groupnodes));
      }
      if ($argument['type'] == 'calendar_year') {
        if (!$view->args[$delta]) $view->args[$delta] = date_format($now, 'Y');
        $query->year = $view->args[$delta];
        calendar_filter_year($query, $query->year);
      }
      elseif (($argument['type'] == 'calendar_month' || $argument['type'] == 'calendar_week')) {
        if (!$view->args[$delta]) $view->args[$delta] = date_format($now, 'm');
        $query->month = $view->args[$delta];
        calendar_filter_month($query, $query->month);
      }
      elseif ($argument['type'] == 'calendar_day') {
      	$query->day = CALENDAR_EMPTY_ARG;
        $view->args[$delta] = CALENDAR_EMPTY_ARG;
      }
    }
  }

  // Either a month or a week argument could occupy the second position of the group
  // this is done so that a single view has the capability to switch between all calendar layouts
  // to make this work we must make some adjustments to the view
  if ($view->build_type == 'page') {
    $GLOBALS['calendar_is_calendar'] = TRUE;

    if (empty($view->args) || !calendar_is_calendar_arg($view)) {

    	// If no arguments are provided, default to current month view.
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
        elseif (!strstr($view->args[$delta], 'W') && $view->build_type == 'page'
          && $view->argument[$delta]['type'] == 'calendar_week') {
          calendar_filter_month($query, $view->args[$delta]);
          $view->argument[$delta]['type'] = 'calendar_month';
          $view->argument[$delta]['id'] = 'calendar_month';
          $view->argument[$delta + 1]['type'] = 'calendar_day';
          $view->argument[$delta + 1]['id'] = 'calendar_day';
        }
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

  // Identify the kind of display we're using for this view.
  // Check first for 'view' in url to get displays set by the switch
  // block, then for 'calendar_display_format' variable to get displays
  // set in the setup, default to normal calendar display.
  if (isset($_GET['view']) && $view->build_type == 'page') {
  	$view->calendar_display = !empty($_GET['view']) ? check_plain($_GET['view']) : 'calendar';
  }
  elseif ($view->build_type == 'page') {
  	$display_formats = calendar_get_formats($view);
  	$view->calendar_display = $display_formats[$view->calendar_type];
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
    if (($item->calendar_start_date >= $view->min_date && $item->calendar_start_date <= $view->max_date)
     || ($item->calendar_end_date >= $view->min_date && $item->calendar_end_date <= $view->max_date)) {
      $values[date_format($item->calendar_start_date, 'Y-m-d')][date_format($item->calendar_start_date, 'Y-m-d')][] = $item;
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
      $len = variable_get('calendar_day_header_'. $view->name, 1);
      $rows = array_merge(array(calendar_week_header($view->mini, $params['with_weekno'], $len)), $rows);
      break;
  }

  // Add the navigation in as the header.
  $header = calendar_nav($view, $params['with_weekno']);
  $output = theme('calendar_links', $view, $view->build_type != 'block');
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
  } while (date_format($curday, 'n') == $month && $curday <= $view->max_date);

  // Merge the day names in as the first row.
  $len = variable_get('calendar_day_header_'. $view->name, 1);
  $rows = array_merge(array(calendar_week_header($view->mini, $params['with_weekno'], $len)), $rows);
  return $rows;
}

/**
 * Build one week row.
 */
function calendar_build_week(&$curday, $view, $items, $params, $check_month = FALSE) {
  $weekdays = calendar_untranslated_days($results, $view);
  $today = date_format(date_now(date_default_timezone_name()), 'Y-m-d');
  $month = date_format($curday, 'n');
  $week = calendar_date_week(date_format($curday, 'Y-m-d'));

  $first_day = variable_get('date_first_day', 0);

  // move backwards to the first day of the week
  $day_wday = date_format($curday, 'w');
  date_modify($curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');

  // If we're displaying the week number, add it as the
  // first cell in the week.
  if ($params['with_weekno'] && $view->calendar_type != 'day') {
    $url = $params['url'] .'/'. $view->year .'/W'. $week;
    $rows[$week][] = array(
      'data' => l($week, $url, array('query' => $params['append'])),
      'class' => 'week');
  }

  for ($i = 0; $i < 7; $i++) {
    if ($check_month && ($curday < $view->min_date || $curday > $view->max_date || date_format($curday, 'n') != $month)) {
      $class = strtolower($weekdays[$i] .
      ($view->mini ? ' mini' : ''));
      $content = theme('calendar_empty');
    }
    else {
      $content = calendar_build_day($curday, $view, $items, $params);
      $class = strtolower($weekdays[$i] .
      (date_format($curday, 'Y-m-d') == $today ? ' today' : '') .
      (date_format($curday, 'Y-m-d') < $today ? ' past' : '') .
      (date_format($curday, 'Y-m-d') > $today ? ' future' : '') .
      ($view->mini ? ' mini' : ''));
    }

    $rows[$week][] = array(
      'data' => $view->mini ? $content : '<div class="inner">'. $content .'</div>',
      'class' => $class, 'id' => $view->name . '-' . date_format($curday, 'Y-m-d'));

    date_modify($curday, '+1 day');
  }

  return $rows;
}

/**
 * Build the contents of a single day for the $rows results.
 */
function calendar_build_day($curday, $view, $items, $params) {
  $nextday = drupal_clone($curday);
  date_modify($nextday, '+1 day');
  $inner = '';
  $selected = FALSE;
  foreach ($items as $date => $day) {
    if ($date == date_format($curday, 'Y-m-d')) {
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
    $inner = theme('calendar_empty');
  }
  $content = theme('calendar_date_box', date_format($curday, 'Y-m-d'), $view, $items, $params, $selected) . $inner;
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
  $translated_days = date_week_days_ordered(date_week_days_abbr(TRUE));

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
    $range = calendar_week_range($query);
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
  include_once(drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
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

    if ($this_field['timestamp_fromto'] || $this_field['string_fromto']) {
      // Format dates with from and to times into a single value that is made
      // up of the two local DATETIME values separated with a pipe (|).
      $value = $this_field['timestamp_fromto'] ? 'timestamp_fromto' : 'string_fromto';
      $dbfield = date_sql_concat(array($this_field[$value][0], "'|'", $this_field[$value][1]));
      $queries[] = "(". $date_handler->sql_where_date('DATE', $this_field[$value][1], '>=', $min) .
        " AND ". $date_handler->sql_where_date('DATE', $this_field[$value][0], '<=', $max) .")";
    }
    else {
      $dbfield = $this_field['fullname'];
      $queries[] = "(". $date_handler->sql_where_date('DATE', $this_field['fullname'], '>=', $min) .
        ' AND '. $date_handler->sql_where_date('DATE', $this_field['fullname'], '<=', $max) .")";
    }
    $query->fields[$field_delta] = $dbfield .' AS '. $this_field['query_name'];
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
  global $user;

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
        $option           = $fields[$data['field']];
        $field_type       = strstr($option['type'], 'string') ? 'string' : 'timestamp';
        $field_field      = $option['query_name'];
        //$field_end        = $field_field .'2';
        $field_field_name = $option['field_name'];
        $timestamp_fromto = $option['timestamp_fromto'];
        $string_fromto    = $option['string_fromto'];
        //$field_id         = $delta;
        $tz_handling      = $option['tz_handling'];
        $label            = $data['label'];
        //$granularity      = $option['granularity'];

        if (strstr($option['type'], 'cck')) {
          $format = date_formatter_format($data['options'], $field_field_name);
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

        $fromto = array_merge(
          $fields[$data['field']]['timestamp_fromto'],
          $fields[$data['field']]['string_fromto']);

        foreach ($items as $delta => $item) {
          // The query created from/to dates like 2007-12-31T01:30:00|2007-12-31T02:30:00,
          // explode those values and separate them into from and to dates on the node.
          if ($item->$field_field) {
            $values = explode('|',  str_replace('T', ' ', $item->$field_field));
            if ($field_type != 'string') {
              $date = date_make_date($values[0], 'UTC', DATE_UNIX);
              $values[0] = date_format($date, DATE_FORMAT_DATETIME);
              $date = date_make_date($values[1], 'UTC', DATE_UNIX);
              $values[1] = date_format($date, DATE_FORMAT_DATETIME);
            }

            // Create a node for each date within the field's date range,
            // limited to the view's date range.
            $now = max(date_format($view->min_date, 'Y-m-d'), substr($values[0], 0, 10));
            $to  = min(date_format($view->max_date, 'Y-m-d'), substr($values[1], 0, 10));
            if (empty($to)) {
              $to = $now;
            }
            while ($now <= $to) {
              $node = drupal_clone($item);
              $node->title = $node->node_title;
              $node->label = $label;
              $node->format = $format;
              $node->format_time = variable_get('calendar_time_format_'. $view->name, 'H:i');
              $node->url = 'node/'. $node->nid;

              // Convert the table.field format in the fromto fields
              // to the table_field format used by the Views formatters.
              $node->{str_replace('.', '_', $fromto[0])} = $values[0];
              $node->{str_replace('.', '_', $fromto[1])} = $values[1];

              // Flag which datefield this node is using, in case
              // there are multiple date fields in the view.
              $node->datefield = $field_field;

              // If there are other datefields in the View, get rid
              // of them in this pseudo node. There should only be one
              // date in each calendar node.
              foreach ($node as $key => $val) {
                if ($key != $field_field && in_array($key, $datefields)) {
                  unset($node->$key);
                }
              }

              $start = $now .' 00:00:00';
              $end = $now .' 23:59:59';
              $node->calendar_start = $values[0] < $start ? $start : $values[0];
              $node->calendar_end = !empty($values[1]) ? ($values[1] > $end ? $end : $values[1]) : $node->calendar_start;

              $node->calendar_start_date = date_create($node->calendar_start, timezone_open('UTC'));
              $node->calendar_end_date = date_create($node->calendar_end, timezone_open('UTC'));
              if ($tz_handling != 'utc' && $tz_handling != 'none') {
                date_timezone_set($node->calendar_start_date, date_default_timezone());
                date_timezone_set($node->calendar_end_date, date_default_timezone());
              }
              $node->calendar_start = date_format($node->calendar_start_date, DATE_FORMAT_DATETIME);
              $node->calendar_end = date_format($node->calendar_end_date, DATE_FORMAT_DATETIME);

              if (isset($node) && (empty($node->calendar_start))) {
                // if no date for the node and no date in the item
                // there is no way to display it on the calendar
                unset($node);
              }
              else {
                if (strstr($option['type'], 'cck')) {
                  $id = $item->nid .':'. $delta .':'. $field_field;
                }
                else {
                  $id = $item->nid .':0:'. $field_field;
                }
                if ($view->build_type == 'page' && $view->calendar_type != 'year') {
                  $node->stripe = calendar_node_stripe($view, $node, $field_field, $field_field);
                }
                $node->unique_nid = $id;
                $nodes[] = $node;
                unset($node);
              }
              $next = date_make_date($now, 'UTC');
              date_modify($next, '+1 day');
              $now = date_format($next, 'Y-m-d');
            }
          }
        }
      }
    }
  }
  return $nodes;
}

/**
 *  Function to construct back and next navigation from views arguments
 */
function calendar_nav($view, $with_weekno = FALSE) {
  $mini = $view->mini;
  if (!calendar_part_is_valid($view->year, 'year')) {
    return array($view->subtitle);
  }

  // make the navigation into a header, with prev and next links
  // use the calendar_nav themes to mimic standard calendar navigation
  $paths = calendar_get_paths($view);
  $prev_path = implode('/', array_reverse($paths[0]));
  $next_path = implode('/', array_reverse($paths[1]));

  $prev_query = $next_query = array();

  if ($_GET['view']) {
    $prev_query[] = 'view='. $_GET['view'];
    $next_query[] = 'view='. $_GET['view'];
  }
  // for the mini calendar in a block, treat the url as a 	 
	// querystring to avoid actually changing the page 	
	if ($mini) { 	 
	  $prev_query[] = 'mini='. $prev_path; 	 
	  $prev_path = $_GET['q']; 	 
	  $next_query[] = 'mini='. $next_path; 	 
	  $next_path = $_GET['q']; 	 
	}
	$prev_query[] = calendar_url_append($view);
  $next_query[] = calendar_url_append($view);

  switch ($view->calendar_type) {
    case 'day':
    case 'year':
      $colspan_prev = 1;
      $colspan_middle = 1;
      $colspan_next = 1;
      break;
    default:
      $colspan_prev = 2;
      $colspan_middle = 3;
      $colspan_next = 2;
      if ($with_weekno) {
        $colspan_prev = 3;
      }
      break;
  }
  $header = array();
  $header[]  = array('data' => theme('calendar_nav_prev', $prev_path, $view->build_type == 'block' ? FALSE : TRUE, implode('&', $prev_query)), 'class' => 'prev', 'colspan' => $colspan_prev);
  $header[]  = array('data' => $view->subtitle, 'class' => 'heading', 'colspan' => $colspan_middle);
  $header[]  = array('data' => theme('calendar_nav_next', $next_path, $view->build_type == 'block' ? FALSE : TRUE, implode('&', $next_query)), 'class' => 'next', 'colspan' => $colspan_next);

  return $header;
}

function calendar_get_paths($view) {
  $path = array();
  $prev_date = drupal_clone($view->min_date);
  date_modify($prev_date, '-1 '. $view->calendar_type);
  $next_date = drupal_clone($view->min_date);
  date_modify($next_date, '+1 '. $view->calendar_type);

  // build an array of the current path and its parts
  $i = 0;
  $path[$i]     = array(
    'path' => $view->url,
    'type' => 'url',
      );
  foreach ($view->argument as $delta => $arg) {
    if ($view->args[$delta]) {
      $i++;
      $pathtype = str_replace('calendar_', '', $arg['type']);
      $path[$i] = array(
        'path' => $view->args[$delta] != CALENDAR_EMPTY_ARG ? $view->args[$delta] : CALENDAR_EMPTY_ARG,
        'type' => $pathtype,
        );
    }
  }

  // if there are other arguments after the view arguments, add them to the
  // navigation links
  while($i < sizeof($view->args)) {
    $i++;
    $path[$i] = array(
      'path' => $view->args[intval($i - 1)],
      'type' => '',
    );
  }

  // reverse through the path, creating a $nextpath and $prevpath arrays
  $formats = array('day' => 'j', 'month' => 'm', 'year' => 'Y', 'week' => 'W');
  for ($x = $i; $x >= 0; $x--) {
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
    else {
      $nextpath[$x] = $path[$x]['path'];
      $prevpath[$x] = $path[$x]['path'];
    }
  }
  return array($prevpath, $nextpath);
}

/**
 * Set the start and end dates for a calendar week.
 */
function calendar_week_range(&$view) {
  if (isset($view->week)) {
    $min_date = date_make_date($view->year .'-01-01 00:00:00', date_default_timezone_name());
    date_timezone_set($min_date, date_default_timezone());

    // move to the right week
    date_modify($min_date, '+' . strval(7*($view->week-1)) . ' days');

    // move backwards to the first day of the week
    $first_day = variable_get('date_first_day', 0);
    $day_wday = date_format($min_date, 'w');
    date_modify($min_date, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');

    // move forwards to the last day of the week
    $max_date = drupal_clone($min_date);
    date_modify($max_date, '+7 days');

    if (date_format($min_date, 'Y') != $view->year) {
      $min_date = date_make_date($view->year .'-01-01 00:00:00', date_default_timezone());
    }

    date_timezone_set($min_date, timezone_open('UTC'));
    date_timezone_set($max_date, timezone_open('UTC'));
    $view->min_date = $min_date;
    $view->max_date = $max_date;
    return array($min_date, $max_date);
  }
  return array();
}