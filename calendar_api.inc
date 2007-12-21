<?php
// $Id$

/**
 * Returns a calendar in the requested format, populated with the provided nodes.
 *
 * @param $type - The format of calendar to return. Possible values:
 *   "month": A month calendar.
 *   "week": A week calendar.
 *   "day": A day calendar.
 *
 *  @param $nodes An associative array of nodes with nids for key values.
 *     Node objects must have date objects adjusted to the local time for
 *     start and end dates in the following format:
 *     $node = (object) {
 *       calendar_start - the local start date and time formatted as Y-m-d H:i:s
 *       calendar_start_date - a date object for the local start date
 *       calendar_end - the local end date and time formated as Y-m-d H:i:s
 *       calendar_end_date - a date object for the local end date
 *       calendar_format - a format string to use on the displayed date
 *       calendar_format_time - a format string to use for a time-only display
 *
 *  @param $module - String containing the name of the module calling the function
 *
 *  @param $title - A string value that will be printed into the header of the calendar
 *
 *  @param $params - an optional array of additional values that may be needed
 *     $params[force_week]  - a week number to limit the displayed calendar to only that week
 *     $params[with_weekno] - add a column for the week number to the calendar
 *     $params[hide_header] - true to omit td header row(s)
 *     $params[as_array]    - true to return the rows array instead of the themed table
 *     $params[mini]        - this is a mini calendar, don't display item details
 *     $params[url]         - an url to use as a base url in month and days
 *     $params[append]      - a query string to append to urls
 *     $params[stripe]      - an node field to use instead of nid for stripes
 *     $params[limit]       - an array of a minimum and maximum timestamp to include on the calendar
                                used in case node array has values that overlap periods
 *  @return Themed calendar view of nodes or array of calendar rows
 */
function calendar_get_calendar($type, $nodes, $module, $title = NULL, $params = array()) {

  $now = date_now();
  if ($params['limit'][0]) {
    $min_date = $params['limit'][0];
    $min_datetime = date_format($min_date, DATE_FORMAT_DATETIME);
    $max_date = $params['limit'][1];
    $max_datetime = date_format($max_date, DATE_FORMAT_DATETIME);
  }

  $data  = array();
  foreach ($nodes as $nid => $node) {

    $node->stripe = $params['stripe'] ? $node->$params['stripe'] : $node->nid;

    $calendar = new stdClass();
    $nodes[$node->nid] = $node;
    $calendar->nid = $node->nid;

    // Limit node start and end dates to the requested date range.
    if ($min_datetime > $node->calendar_start) {
      $node->calendar_start = $min_datetime;
      $node->calendar_start_date = $min_date;
    }
    if ($max_datetime  < $node->calendar_end) {
      $node->calendar_end = $max_datetime;
      $node->calendar_end_date = $max_date;
    }

    // Test whether the days are the same.
    $test_start = substr($node->calendar_start, 0, 10);
    $test_end = substr($node->calendar_end, 0, 10);

    if ($test_start == $test_end) {
      $calendar = drupal_clone($node);
      $calendar->calendar_state = 'singleday';
      $calendar->date = $node->calendar_start_date;
      $calendar->formatted = date_format($node->calendar_start_date, $node->format);
      $calendar->formatted_time = date_format($node->calendar_start_date, $node->format_time);
      $data[date_format($node->calendar_start_date, 'Y')][date_format($node->calendar_start_date, 'm')][date_format($node->calendar_start_date, 'j')][] = $calendar;
    }
    else {
      // roll through each day the event occurs and set an entry for each
      $date = drupal_clone($node->calendar_start_date);
      $x = date_format($date, 'Y-m-d');
      while ($x <= substr($node->calendar_end, 0, 10)) {
        $calendar = drupal_clone($node);
        if ($x == $test_end) {
          $calendar->calendar_state = 'end';
        }
        elseif ($x == $test_start) {
          $calendar->calendar_state = 'start';
        }
        else {
          $calendar->calendar_state = 'ongoing';
        }
        $calendar->date = $date;
        $calendar->formatted = date_format($date, $node->format);
        $calendar->formatted_time = date_format($date, $node->format_time);
        // Store this date and go on to next day.
        $data[date_format($date, 'Y')][date_format($date, 'm')][date_format($date, 'j')][] = $calendar;
        date_modify($date, '+1 day');
        $x = date_format($date, 'Y-m-d');
      }
    }
  }

  // order the years, months and days
  ksort($data, SORT_NUMERIC);
  foreach($data as $year => $months) {
    ksort($data[$year], SORT_NUMERIC);
    foreach($data[$year] as $month => $days) {
      ksort($data[$year][$month], SORT_NUMERIC);
    }
  }

  $weekdays = calendar_untranslated_days();

  switch ($type) {
    case 'day':
    case 'table':
      foreach ($data as $year => $months) {
        if(count($data) > 1 && !$params['hide_header']) {
          // add year heading
          $rows[][] = array(
            'class' => 'heading year',
            'id' => 'year'.$year,
            'data' => $year);
        }
        foreach($months as $month => $days) {
          foreach($days as $day => $calendars) {
            $content = theme('calendar_date_box', $year, $month, $day, 'table', $params, !empty($calendars));
            foreach($calendars as $calendar) {
              if(!$month_name) {
                $month_name = date_format($calendar->date, 'M');
                $dow = _calendar_day_of_week($calendar->date);
              }
              $node = $nodes[$calendar->nid];

              $node->calendar_state = $calendar->calendar_state;

              if($output = module_invoke($module, 'calendar_node_'. $type, $node)) {
                $content .= $output;
              }
              else {
                $content .= theme('calendar_node_'. $type, $node);
              }
            }
            $rows[][] = array(
              'class' => strtolower("$month_name ". $weekdays[$dow] . ($calendar->stamp == $today ? ' today' : '')),
              'id' => strtolower($month_name . $day),
              'data' => $content);
            $month_name = NULL;
          }
        }
      }
    break;

    case 'week':
    case 'month':
    case 'year':
      $colspan = $params['with_weekno'] ? '8' : '7';
      foreach ($data as $year => $months) {
          $month_rows[] = $title;
          if(count($data) > 1 && !$params['hide_header']) {
            // add year heading
            $rows[][] = array(
              'class' => 'heading year',
              'id' => 'year'. $year,
              'data' => $year,
              'colspan' => $colspan);
          }
          foreach ($months as $month => $days) {
            // Date object for first day in month
            $curday = date_create($year .'-'. date_pad($month) .'-01', date_default_timezone());
            // Date object for last day in month
            $lastday = drupal_clone($curday);
            date_modify($lastday, '+1 month');
            date_modify($lastday, '-1 second');

            // pad the first week row array to fill up days in the previous month we don't build
            $row = array_fill(0, 6, theme('calendar_empty_day'));
            // get the day of week offset value for the first day of the month
            $start = $offset = _calendar_day_of_week($curday);
            // get name of month
            $month_name = date_format($curday, 'M');
            $month_link = $params['url'] ? l($month_name, $params['url'] .'/'. $year .'/'. intval($month), NULL, $params['append']) : $month_name;
            // set week counter
            $week = 0;
            if (!$params['hide_header']) {
              $rows[][] = array(
                'class' => 'heading month',
                'id' => 'month'. $month,
                'data' => $month_name,
                'colspan' => $colspan);
            }
            $rows[] = calendar_week_header($params['mini'], $params['with_weekno']);

            while ($curday < $lastday) {
              for ($x = $start; $x < 7; $x++) {
                $mday = (($week * 7) + ($x + 1) - $offset);
                $selected = FALSE;
                if(is_array($days[$mday])) {
                  // make sure dummy nodes added to create blank calendars don't get marked as having content
                  foreach ($items = $days[$mday] as $item) {
                    if ($item->nid) {
                      $selected = TRUE;
                    }
                  }
                }
                $content = theme('calendar_date_box', $year, $month, $mday, $type, $params, $selected);

                // render nodes for the day
                if(is_array($days[$mday]) && !$params['mini']) {
                  foreach($days[$mday] as $calendar) {
                    $node = $nodes[$calendar->nid];
                    $node->calendar_state = $calendar->calendar_state;

                    if($output = module_invoke($module, 'calendar_node_'. $type, $node)) {
                      $content .= $output;
                    }
                    elseif (!$params['mini'] && $node->nid != 0) {
                      $content .= theme('calendar_node_'. $type, $node);
                    }
                  }
                }
                $row[$x] = array(
                  'class' => strtolower("$month_name ". $weekdays[$x] . ($curday == $today ? ' today' : '') . ($params['mini'] ? ' mini' : '')),
                  'id' => strtolower($month_name . $mday),
                  'data' => $params['mini'] ? $content : '<div class="inner">'. $content .'</div>');

                date_modify($curday, '+1 day');
                if (date_format($curday, DATE_FORMAT_DATETIME) > date_format($lastday, DATE_FORMAT_DATETIME)) {
                  $x = 8;
                }
              }

              // We finished up a week, and went into the next one,
              // so clone the date and back up to get week info.
              $viewday = drupal_clone($curday);
              date_modify($viewday, '-1 day');
              $view = new StdClass();
              $view->year = date_format($viewday, 'Y');
              $view->month = date_format($viewday, 'n');
              $view->day = date_format($viewday, 'j');
              calendar_week_range($view);

              // print week unless this is a week view and force_week is set and this is not the forced week
              if (!($type == 'week' && $params['force_week'] && $params['force_week'] != $view->week)) {
                if ($params['with_weekno']) {
                  $week_row = array(0 =>
                    array(
                      'class' => 'week',
                      'data' => l(date_pad($view->week), $params['url'] .'/'. $view->week_year .'/W'. date_pad($view->week), NULL, $params['append']),
                    ));
                  $rows[] = array_merge($week_row, array_pad($row, 7, theme('calendar_empty_day')));
                }
                else {
                  $rows[] = array_pad($row, 7, theme('calendar_empty_day'));
                }
              }

              $week++;
              $start = 0;
              $row = array();
            }
            if ($type == 'year' && !$params['as_array']) {
              $header = array($month_name ? array('class' => 'heading', 'data' => $month_link, 'colspan' => $colspan) : array());
              $output = theme('calendar_month', 'page', $header, $rows);
              unset($rows);
              $month_rows[] = $output;
            }
          }
        }
      break;
  }

  // if only the rows array is requested, return it
  if ($params['as_array']) return $rows;
  $header = $title ? array(array('class' => 'heading', 'data' => $title, 'colspan' => $colspan)) : array();

  $op = $params['mini'] ? 'mini' : 'page';
  return theme('calendar_'. $type, $op, $header, $rows, $month_rows);
}

/**
 *  Implementation of hook_calendar_node() from the calendar_get_calendar() api
 *  calendar api is expecting a function for each calendar type but
 *  all of them need the same processing, so run them through a single function
 *  instead of duplicating the processing for each one
 */
function calendar_calendar_node_month($node) {
  return calendar_calendar_node($node, 'calendar_node_month');
}
function calendar_calendar_node_day($node) {
  return calendar_calendar_node($node, 'calendar_node_day');
}
function calendar_calendar_node_week($node) {
  return calendar_calendar_node($node, 'calendar_node_week');
}
function calendar_calendar_node($node, $type) {
  if ($node->nid && $node->nid !== 0 && $node->calendar_start_date) {
    // this is a real calendar, go ahead and display it
    if (!$node->remote) {
      // switch our psuedo nids back to the right values
      $tmp = explode(':', $node->nid);
      $node->nid = $tmp[0];
      $node->instance = $tmp[1];
    }
    if (isset($node->calendar_node_theme)) {
      return theme($node->calendar_node_theme, $node, $type);
    }
    else {
      return theme('calendar_calendar_node', $node, $type);
    }

  } else {
    // Surpress display of psuedo calendars added to force display of a blank
    // calendar. Have to return some value for blank day so not overridden by
    // normal calendar node theme. A blank space seems to be sufficient.
    return ' ';
  }
}

/**
 * Formats the weekday information into table header format
 *
 * @ingroup event_support
 * @return array with weekday table header data
 */
function calendar_week_header($mini = FALSE, $with_week = TRUE) {
  // create week header
  $untranslated_days = calendar_untranslated_days();
  $translated_days = date_week_days_ordered(date_week_days_abbr(TRUE));

  if ($with_week) {
    $row[] = array('class' => "days week", 'data' => '&nbsp;');
  }
  foreach ($untranslated_days as $delta => $day) {
    $label = $mini ? drupal_substr($translated_days[$delta], 0 , 1) : $translated_days[$delta];
    $row[] = array('class' => "days ". $day, 'data' => $label);
  }
  return $row;
}
/**
 * Return the day of week with start of week offset applied
 * @param $date a date object
 * @return integer day of the week
 */
function _calendar_day_of_week($date) {
	$dow = date_format($date, 'w');
  if ($dow - variable_get('date_first_day', 1) >= 0 ) {
    $dow = $dow - variable_get('date_first_day', 1);
  }
  else {
     $dow = 7 - variable_get('date_first_day', 1);
  }
  return $dow;
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
  } else {
    $month = calendar_part_is_valid($query->month, 'month') ? $query->month : 1;
    $day   = calendar_part_is_valid($query->day, 'day') ? $query->day : 1;
    $year  = calendar_part_is_valid($query->year, 'year') ? $query->year : date_format($now, 'Y');
    $date  = date_create($year .'-'. date_pad($month) .'-'. date_pad($day) .' 00:00:00', date_default_timezone());
  }
  $query->min_date = $date;
  $max_date = drupal_clone($date);
  date_modify($max_date, '+1 '. $query->calendar_type);
  date_modify($max_date, '-1 second');
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

  if (array_key_exists($field_name, $fields)) {
    $query->ensure_table($this_field['table'], $this_field['table']);
    $tz_handling  = $this_field['tz_handling'];
    $offset_field = $this_field['offset_field'];
    $field_type   = strstr($this_field['type'], 'string') ? 'iso' : 'int';

    // Figure out the appropriate timezone offset to use to convert the date
    // back to local time.
    switch ($tz_handling) {
      case 'user' :
       global $user;
        $start_offset = $end_offset = $user->timezone;
        break;
        $start_offset = $end_offset = 0;
        break;
      case 'date' :
        $start_offset = $end_offset = $offset_field;
        break;
      // Event-specific timezones can't be converted in the sql because no offset
      // is stored in the database, so retrieve the UTC value and convert
      // it in the node processing.
      case 'event' :
      case 'GMT':
        $start_offset = $end_offset = 0;
        break;
      case 'none':
      default :
        $start_offset = $end_offset = variable_get('date_default_timezone', 0);
        break;
    }

    // Create minimum and maximum comparison dates in DATETIME format.
    $min_compare = date_format($query->min_date, DATE_FORMAT_DATETIME);
    $max_compare = date_format($query->max_date, DATE_FORMAT_DATETIME);

    // Figure out where this field is in the query's field array
    // so we know which query field to adjust.
    foreach ($query->fields as $delta => $query_field) {
      if (strstr($query_field, $this_field['fullname'] .' AS')) {
        $field_delta = $delta;
      }
    }

    // Create SQL that will alter the retrieved field to reformat the
    // date to a DATETIME field in the local timezone. In the case of
    // dates with from and to times, format a single value that is made
    // up of the two local DATETIME values separated with a pipe (|).

    if ($this_field['timestamp_fromto']) {
      $adjusted_field = date_sql_concat(array(
        date_sql('DATE', $this_field['timestamp_fromto'][0], $field_type, $start_offset),
        "'|'",
        date_sql('DATE', $this_field['timestamp_fromto'][1], $field_type, $end_offset),
        ));
      $query->fields[$field_delta] = $adjusted_field .' AS '. $this_field['query_name'];
      $queries[] = "(SUBSTRING(". $adjusted_field .", -19) >='". $min_compare .
        "' AND SUBSTRING(". $adjusted_field .", 1, 19) <='". $max_compare ."')";
      $event_field_processed = TRUE;
    }
    elseif ($this_field['string_fromto']) {
      $adjusted_field = date_sql_concat(array(
        date_sql('DATE', $this_field['string_fromto'][0], $field_type, $start_offset),
        "'|'",
        date_sql('DATE', $this_field['string_fromto'][1], $field_type, $end_offset),
        ));
      $query->fields[$field_delta] = $adjusted_field .' AS '. $this_field['query_name'];
      $queries[] = "(SUBSTRING(". $adjusted_field .", -19) >='". $min_compare .
        "' AND SUBSTRING(". $adjusted_field .", 1, 19) <='". $max_compare ."')";
      $event_field_processed = TRUE;
    }
    // handling for single day dates
    else {
      $adjusted_field = date_sql('DATE',$this_field['fullname'], $field_type, $start_offset);
      $queries[] = "(". $adjusted_field .">='". $min_compare .
        "' AND ". $adjusted_field ."<='". $max_compare ."')";
    }

    // We reconstructed the query to pull the current date out of the database
    // and to format from/to dates into a single value, so we can simplify the
    // query to eliminate all the related fields that were used identify the
    // field todate, timezone, and offset.
    foreach ($query->fields as $delta => $query_field) {
      $test = explode(' AS ', $query_field);
      if ($test[0] != $this_field['fullname'] && in_array($test[0], $this_field['related_fields'])) {
        unset($query->fields[$delta]);
      }
    }
  }
  return $queries;
}

/**
 * The workhorse function that takes the beginning array of items and alters it
 *   to an array of calendar nodes that the theme can handle.
 */
function calendar_get_nodes(&$view, &$items, $type) {
  $view->nodes_per_page = 0;
  $type_names = node_get_types('names');

  unset($params['limit']);

  if ($type == 'block' || $view->calendar_type == 'year') $params['mini'] = TRUE;

  $fields = calendar_fields();
  $calendar_fields = (array) array_keys($fields);
  $nodes = array();
  $i = 0;
  $items_in = $items;

  // explode out field and format info from the view
  foreach ($view->field as $delta => $data) {
    if (in_array($data['field'], $calendar_fields)) {
      $option           = $fields[$data['field']];
      $field_type       = strstr($option['type'], 'string') ? 'string' : 'timestamp';
      $field_field      = $option['query_name'];
      $field_end        = $field_field .'2';
      $field_field_name = $option['field_name'];
      $timestamp_fromto = $option['timestamp_fromto'];
      $string_fromto    = $option['string_fromto'];
      $field_id         = $delta;
      $tz_handling      = $option['tz_handling'];
      $label            = $data['label'];
      $granularity      = $option['granularity'];
      $view_fields      = _views_get_fields();

      if (strstr($option['type'], 'cck')) {
        $format = date_formatter_format($data['options'], $field);
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

      // Iterate through the $items array returned by the query and create
      // date or pseudo date nodes.
      foreach ($items as $delta => $item) {
        $node = drupal_clone($item);
        $node->title = $node->node_title;
        $node->label = $label;
        $node->format = $format;
        $node->format_time = variable_get('calendar_time_format_'. $view->name, 'H:i');
        foreach ($view->field as $field) {

          // Get the themed label for the field from a Views theme.
          if (!in_array($field['field'], $calendar_fields) && $field['field'] != 'title') {
            if ($view_fields[$field['id']]['visible'] !== FALSE) {
              $node->fields[$field['queryname']] = views_theme_field('views_handle_field', $field['queryname'], $view_fields, $field, $node, $view);
            }
          }
        }

        // Create date objects for the start and end time of the node.
        $dates = explode('|', $item->$field_field);
        $node->calendar_start = $dates[0];
        $node->calendar_end   = !empty($dates[1]) ? $dates[1] : $dates[0];


        // If we're dealing with an event node that uses event-specific timezones
        // we couldn't transform the date to the local time in the query
        // so we have a date in UTC and need to convert it to local time here.
        if (module_exists('event')
        && ($field_field == 'event_event_start' || $field_field == 'event_event_end')
        && $tz_handling == 'event') {
          $dates = explode('|', $item->$field_field);
          $node->calendar_start_date = date_create($node->calendar_start, timezone_open('UTC'));
          date_modify($date_start, '+'. $node->event_timezone .' seconds');
          $node->calendar_start_date = $date_start;
          $node->calendar_end_date = date_create($node->calendar_end, timezone_open('UTC'));
          date_modify($node->calendar_end_date, '+'. $node->event_timezone .' seconds');
        }
        else {
          $node->calendar_start_date = date_create($node->calendar_start, date_default_timezone());
          $node->calendar_end_date   = date_create($node->calendar_end, date_default_timezone());
        }
        if (isset($node) && (empty($node->calendar_start_date))) {
          // if no date for the node and no date in the item
          // there is no way to display it on the calendar
          unset($node);
        }
        else {
          // we can have multiple representations with the same nid, like
          // multi-day values or different fields that refer to the same node
          // create a unique id so it gets displayed in the calendar
          // Add delta to key to make multiple value CCK fields display
          // as separate items.
          if (strstr($option['type'], 'cck')) {
            $id = $item->nid .':'. $delta .':'. $field_field;
          }
          else {
            $id = $item->nid .':0:'. $field_field;
          }
          $node->nid = $id;

          if ($view->build_type == 'page' && $view->calendar_type != 'year') {
            $node->stripe = calendar_node_stripe($view, $node, $option['query_name'], $field_field);
          }
          $nodes[$id] = $node;
          unset($node);
        }
      }
    }
  }

  // make sure there is at least one item in $nodes to force the calendar to
  // display set the hour to 12 to minimize timezone adjustments that might
  // push it to previous or next day

  if ($view->calendar_type == 'year') {
    // for the year view to work, must have at least one node in each month
    for ($i=1; $i<13; $i++) {
      $nodes[] = _calendar_make_node(NULL, $view->year, $i, 1, 12, 0);
    }
  }
  elseif ($view->calendar_type == 'week') {
    // make sure at least one node is created for the current week
    // add both start and end of week in case week crosses from one month to next
    $week_range = calendar_week_range($view);
    date_time_set($week_range[0], 12, 0, 0);
    date_time_set($week_range[1], 12, 0, 0);
    $nodes[]    = _calendar_make_node($week_range[0]);
    $nodes[]    = _calendar_make_node($week_range[1]);
  }
  elseif (sizeof($nodes) == 0) {
    // otherwise add a blank node for the current day
    $nodes = array(_calendar_make_node(NULL, $view->year, $view->month, $view->day, 12, 0));
  }

  if (calendar_part_is_valid($view->year, 'year')) {

    // valid year is a test that indicates if arguments were available to
    // establish a date for the calendar a full view with an argument date
    // will display a single month, day or week calendar page with navigation
    // that mimics regular calendar

    // trim off date values that are outside the selected range to prevent
    // display of incomplete extra calendars
    $min_datetime = date_format($view->min_date, DATE_FORMAT_DATETIME);
    $max_datetime = date_format($view->max_date, DATE_FORMAT_DATETIME);
    foreach ($nodes as $delta => $node) {
      if (date_format($node->calendar_end_date, DATE_FORMAT_DATETIME) < $min_datetime
      || date_format($node->calendar_start_date, DATE_FORMAT_DATETIME) > $max_datetime) {
        unset($nodes[$delta]);
      }
    }

    $params['limit'] = array($view->min_date, $view->max_date);

    // hide the intermediate header rows created by the event api and
    // push title and navigation into calendar header
    $params['hide_header'] = $view->calendar_type == 'week' ? false : true;
    //$title = theme('calendar_nav_wrapper', calendar_nav($view, $params['mini']), array());

    // standard api displays a whole month instead of a single week
    // adjust here for single week display
    if ($view->calendar_type == 'week' && $view->week) {
      $params['force_week'] = $view->week;
    }
  }
  // use calendar_get_calendar api to draw the calendar
  $params['url'] = calendar_real_url($view, $view->args);
  $params['append'] = calendar_url_append($view);
  $params['stripe'] = 'stripe';
  $params['with_weekno'] = $view->build_type == 'block' || $view->calendar_type == 'year' || $view->year < 1970 ? FALSE : TRUE;
  return array('nodes' => $nodes, 'params' => $params);
}

/**
 *  A function to create a blank date to force a calendar display when
 * // there is no data
 */
function _calendar_make_node($date = NULL, $year = NULL, $month = NULL, $day = NULL, $hour = NULL, $minute = NULL) {
  if (!$date) {
    $now = date_now();
    $year   = calendar_part_is_valid($year, 'year')     ? $year   : date_format($now, 'Y');
    $month  = calendar_part_is_valid($month, 'month')   ? $month  : date_format($now, 'm');
    $day    = calendar_part_is_valid($day, 'day')       ? $day    : date_format($now, 'j');
    $hour   = calendar_part_is_valid($hour, 'hour')     ? $hour   : date_format($now, 'H');
    $minute = calendar_part_is_valid($minute, 'minute') ? $minute : date_format($now, 'i');
    while (!checkdate($month, $day, $year)) $day--; // Account for days that don't exist
    $date = date_create($year .'-'. $month .'-'. $day .' '. $hour .':'. $minute .':00', date_default_timezone());
  }
  $node = new stdClass();
  $node->nid = 0;
  $node->calendar_start_date = $date;
  $node->calendar_end_date   = $date;
  return $node;
}

/**
 *  Function to construct back and next navigation from views arguments
 */
function calendar_nav($view, $mini = FALSE) {
  if (!calendar_part_is_valid($view->year, 'year')) {
    return $view->subtitle;
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
  // for the mini calendar in a block, treat the url as a querystring to avoid
  // actually changing the page
  if ($mini && $view->calendar_type == 'month') {
    $prev_query[] = 'mini='. $prev_path;
    $prev_path = $_GET['q'];
    $next_query[] = 'mini='. $next_path;
    $next_path = $_GET['q'];
  }
  $prev_query[] = calendar_url_append($view);
  $next_query[] = calendar_url_append($view);

  $header = array();
  $header[]  = array('data' => theme('calendar_nav_prev', $prev_path, $mini ? FALSE : TRUE, implode('&', $prev_query)), 'class' => 'prev');
  $header[]  = array('data' => $view->subtitle, 'class' => 'heading', 'colspan' => 5);
  $header[]  = array('data' => theme('calendar_nav_next', $next_path, $mini ? FALSE : TRUE, implode('&', $next_query)), 'class' => 'next');

  return $header;
}

function calendar_get_paths($view) {
  $path = array();

  $prev_date = drupal_clone($view->min_date);
  date_modify($prev_date, '-1 '. $view->calendar_type);
  $next_date = drupal_clone($prev_date);
  date_modify($next_date, '+2 '. $view->calendar_type);

  // build an array of the current path and its parts
  $i = 0;
  $path[$i]     = array(
    'path' => $view->real_url,
    'type' => 'url',
      );
  foreach ($view->argument as $delta => $arg) {
    if ($view->args[$delta]) {
      $i++;
      $pathtype = str_replace('calendar_', '', $arg['type']);
      $path[$i] = array(
        'path' => $view->args[$delta] != CALENDAR_EMPTY_ARG ? $view->$pathtype : CALENDAR_EMPTY_ARG,
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

      // Weeks are messy, we can't use format strings to find the calendar week
      // and the year may change along with the week.
      if ($type == 'week') {
        $nextview = new StdClass();
        $nextview->year = date_format($next_date, 'Y');
        $nextview->month = date_format($next_date, 'n');
        $nextview->day = date_format($next_date, 'j');
        $range = calendar_week_range($nextview);
        $nextpath[$x] = 'W'. intval($nextview->week + 1);
        if ($nextview->week_year != date_format($next_date, 'Y')) {
          $next_year = $nextview->week_year;
        }

        $prevview = new StdClass();
        $prevview->year = date_format($prev_date, 'Y');
        $prevview->month = date_format($prev_date, 'n');
        $prevview->day = date_format($prev_date, 'j');
        $range = calendar_week_range($prevview);
        $prevpath[$x] = 'W'. intval($prevview->week + 1);
        if ($prevview->week_year != date_format($prev_date, 'Y')) {
          $prev_year = $prevview->week_year;
        }
      }
      else {
        $nextpath[$x] = $type == 'year' && isset($next_year) ? $next_year : date_format($next_date, $formats[$type]);
        $prevpath[$x] = $type == 'year' && isset($prev_year) ? $prev_year : date_format($prev_date, $formats[$type]);
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
 * Get the start and end dates for a calendar week,
 * and adjust the view with the right week and week year numbers.
 *
 * Tracking the right calendar week number so it works no matter
 * which is the first day of the week, and as you cross from one
 * year to another is quite complex. There are no simple PHP
 * functions that will accomplish this.
 */
function calendar_week_range(&$view) {
  $now = date_now();
  $year = $view->year;
  $view->week_year = $year;
  $day = !empty($view->day) && $view->day != CALENDAR_EMPTY_ARG ? $view->day : date_format($now, 'j');
  $month = !empty($view->month) && $view->month != CALENDAR_EMPTY_ARG ? $view->month : date_format($now, 'n');

  // Create an object for the first day of the week of the first
  // calendar week of this year.
  $first_dow = variable_get('date_first_day', 0);
  $min_date = date_create($view->year .'-01-01 00:00:00', date_default_timezone());

  $diff = -((7 - $first_dow + date_format($min_date, 'w')) % 7);
  date_modify($min_date, $diff .' days');

  if (empty($view->week)) {
    $datetime = $year .'-'. date_pad($month) .'-'. date_pad($day) .' 00:00:00';
    $viewdate = date_create($datetime, date_default_timezone());
    // Special case when week min date is in a prior year.
    $adj = date_format($min_date, 'Y') < $year ? 0 : 1;
    $week_diff = date_diff($min_date, $viewdate, 'weeks');
    $view->week = $week_diff + $adj;
  }

  date_modify($min_date, '+'. ($view->week - 1) .' weeks');
  $max_date = drupal_clone($min_date);
  date_modify($max_date, '+1 week');
  date_modify($max_date, '-1 second');

  // Special case when crossing from one year to another, it could
  // either be computed as the last week of the previous year or first
  // week of the current year, so force the first week of the current year.
  if (date_format($max_date, 'z') < 6) {
    $view->week = 1;
    $view->week_year = $view->year + 1;
  }
  $view->min_date = $min_date;
  $view->max_date = $max_date;
  return array($min_date, $max_date);
}
