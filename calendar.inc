<?php
//$Id$
/**
 * @file
 * All the code used while processing a calendar is stored in this file
 * and is included only when needed.
 */
/**
 * Build calendar from calendar_templates table
 *
 * @param unknown_type $view
 * @param unknown_type $items
 * @param unknown_type $params
 * @return themed table
 */
function calendar_build_calendar($view, $items, $params) {
  // Create minimum and maximum comparison dates in DATETIME format.
  $params['min_compare'] = date_format($view->min_date, DATE_FORMAT_DATETIME);
  $params['max_compare'] = date_format($view->max_date, DATE_FORMAT_DATETIME);

  // Remove nodes outside the selected date range. Group the items by 'm-d'
  // to match the template column value. Subgroup by complete date and time
  // to sort the values within a day.
  $values = array();
  foreach ($items as $item) {
    if ($item->calendar_start >= $params['min_compare'] && $item->calendar_end <= $params['max_compare']) {
      $values[date_format($item->calendar_start_date, 'm-d')][date_format($item->calendar_start_date, 'Y-m-d H:i:s')][] = $item;
    }
  }
  $items = $values;

  // Find the right template and pull it out of the calendar_templates table.
  $rows = array();
  $template = calendar_year_template($view->year);
  $view->template = $template;
  $curday = drupal_clone($view->min_date);

  switch ($view->calendar_type) {
    // The year view is built from the 12 templates with
    // all the week templates in each month.
    case 'year':
      $rows = array();
      for ($i = 1; $i <= 12; $i++) {
        $results = db_query("SELECT * FROM {calendar_templates} WHERE template = %d AND (start LIKE '%s%' OR end LIKE '%s%') ORDER BY weekno", $template, date_pad($i) .'-', date_pad($i) .'-');
        $min = $view->year .'-'. date_pad($i) .'-01 00:00:00';
        $max = $view->year .'-'. date_pad($i) .'-'. date_pad(date_days_in_month($min, DATE_DATETIME)).' 23:59:59';
        $params['min_compare'] = $min;
        $params['max_compare'] = $max;
        $rows[$i] = calendar_build_weeks($curday, $view, $items, $results, $params);
      }
      break;

    // The month view is built from all the week templates in the month.
    case 'month':
      $results = db_query("SELECT * FROM {calendar_templates} WHERE template = %d AND (start LIKE '%s%' OR end LIKE '%s%') ORDER BY weekno", $template, date_format($view->min_date, 'm-'), date_format($view->max_date, 'm-'));
      $rows = calendar_build_weeks($curday, $view, $items, $results, $params);
      break;

    // The day view needs no template.
    case 'day':
      $rows = calendar_build_day(date_pad($view->month) .'-'. date_pad($view->day), $view, $items, $params);
      break;

    // The week view is built from a single week template
    // that might span more than one year.
    case 'week':
      $min_year = date_format($view->min_date, 'Y');
      $template = calendar_year_template($min_year);
      $max_year = date_format($view->max_date, 'Y');
      if ($min_year != $max_year) {
        $template2 = calendar_year_template($max_year);
        $view->template = $template2;
        $results = db_query("SELECT * FROM {calendar_templates} WHERE (template = %d AND end >= '%s') OR (template = %d AND start <= '%s')", $template, date_format($view->min_date, 'm-d'), $template2, date_format($view->max_date, 'm-d'));
      }
      else {
        $results = db_query("SELECT * FROM {calendar_templates} WHERE template = %d AND end >= '%s' AND start <= '%s'", $template, date_format($view->min_date, 'm-d'), date_format($view->max_date, 'm-d'));
      }
      $rows = calendar_build_weeks($curday, $view, $items, $results, $params);
      break;
  }

  // Add the navigation in as the header.
  $header = calendar_nav($view, $view->mini, $params['with_weekno']);
  $output = theme_calendar_links($view, $view->build_type != 'block');
  $output .= theme('calendar_'. $view->calendar_type, $view, $header, $rows);
  return $output;
}

/**
 * Build one or more week rows from a template.
 */
function calendar_build_weeks(&$curday, $view, $items, $results, $params) {
  $weekdays = calendar_untranslated_days($results, $view);
  $prev_month = date_format($curday, 'n');
  $today = date_format(date_now(), 'Y-m-d');
  while ($result = db_fetch_array($results)) {
    // If we're displaying the week number, add it as the
    // first cell in the week.
    if ($params['with_weekno'] && $view->calendar_type != 'day') {
      $url = $params['url'] .'/'. $view->year .'/W'. $result['weekno'];
      $rows[$result['weekno']][] = array(
        'data' => l($result['weekno'], $url, NULL, $params['append']),
        'class' => 'week');
    }
    for ($i = 1; $i <= 7; $i++) {
      $key = 'col'. $i;
      $date = $result[$key];
      $content = '';
      $selected = FALSE;
      if (empty($date)) {
        $content = theme('calendar_empty');
      }
      else {
        $content = calendar_build_day($date, $view, $items, $params);
        date_modify($curday, '+1 day');
      }
      $class = strtolower($weekdays[$i - 1] . (date_format($curday, 'Y-m-d') == $today ? ' today' : '') . ($view->mini ? ' mini' : ''));
      $rows[$result['weekno']][] = array(
        'data' => $view->mini ? $content : '<div class="inner">'. $content .'</div>',
        'class' => $class, 'id' => $date);
    }
  }
  // Merge the day names in as the first row.
  $rows = array_merge(array(calendar_week_header($view->mini, $params['with_weekno'])), $rows);
  return $rows;
}

/**
 * Build the contents of a single day for the $rows results.
 */
function calendar_build_day($date, $view, $items, $params) {
  $datetime = $view->year .'-'. $date;
  $parts = explode('-', $date);
  $inner = '';
  $selected = FALSE;
  if ($datetime .' 00:00:00' >= $params['min_compare'] && $datetime .' 23:59:59' <= $params['max_compare']) {
    if (array_key_exists($date, $items)) {
      $selected = TRUE;
      if (!$view->mini) {
        $nodes = $items[$date];
        ksort($nodes);
        foreach ($nodes as $item) {
          foreach ($item as $node) {
            $theme = isset($node->calendar_node_theme) ? $node->calendar_node_theme : 'calendar_node_'. $view->calendar_type;
            $inner .= theme($theme, $node, $view->calendar_type);
          }
        }
      }
    }
    else {
      $inner = theme('calendar_empty');
    }
    $content = theme('calendar_date_box', $datetime, $view, $params, $selected) . $inner;
  }
  else {
    $content = theme('calendar_empty');
  }
  return $content;
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
    $row[] = array('header' => TRUE, 'class' => "days week", 'data' => '&nbsp;');
  }
  foreach ($untranslated_days as $delta => $day) {
    $label = $mini ? drupal_substr($translated_days[$delta], 0 , 1) : $translated_days[$delta];
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

  // Create minimum and maximum comparison dates in DATETIME format.
  $params['min_compare'] = date_format($query->min_date, DATE_FORMAT_DATETIME);
  $params['max_compare'] = date_format($query->max_date, DATE_FORMAT_DATETIME);

  if (array_key_exists($field_name, $fields)) {
    $query->ensure_table($this_field['table'], $this_field['table']);
    $tz_handling  = $this_field['tz_handling'];
    $offset_field = $this_field['offset_field'];
    $field_type   = strstr($this_field['type'], 'string') ? DATE_ISO : DATE_UNIX;

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
      $queries[] = "(SUBSTRING(". $adjusted_field .", -19) >='". $params['min_compare'] .
        "' AND SUBSTRING(". $adjusted_field .", 1, 19) <='". $params['max_compare'] ."')";
      $event_field_processed = TRUE;
    }
    elseif ($this_field['string_fromto']) {
      $adjusted_field = date_sql_concat(array(
        date_sql('DATE', $this_field['string_fromto'][0], $field_type, $start_offset),
        "'|'",
        date_sql('DATE', $this_field['string_fromto'][1], $field_type, $end_offset),
        ));
      $query->fields[$field_delta] = $adjusted_field .' AS '. $this_field['query_name'];
      $queries[] = "(SUBSTRING(". $adjusted_field .", -19) >='". $params['min_compare'] .
        "' AND SUBSTRING(". $adjusted_field .", 1, 19) <='". $params['max_compare'] ."')";
      $event_field_processed = TRUE;
    }
    // handling for single day dates
    else {
      $adjusted_field = date_sql('DATE',$this_field['fullname'], $field_type, $start_offset);
      $queries[] = "(". $adjusted_field .">='". $params['min_compare'] .
        "' AND ". $adjusted_field ."<='". $params['max_compare'] ."')";
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
 * Take the array of items and alter it to an array of
 * calendar nodes that the theme can handle.
 */
function calendar_build_nodes(&$view, &$items) {
  $view->nodes_per_page = 0;
  $type_names = node_get_types('names');

  $fields = calendar_fields();
  $field_names = (array) array_keys($fields);
  $nodes = array();
  $i = 0;
  $items_in = $items;

  // explode out field and format info from the view
  foreach ($view->field as $delta => $data) {
    if (in_array($data['field'], $field_names)) {
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

        if ($fields[$data['id']]['visible'] !== FALSE && array_key_exists($data['field'], $fields)) {
          // The query created from/to dates like 2007-12-31T01:30:00|2007-12-31T02:30:00,
          // explode those values and separate them into from and to dates on the node.
          $fromto = array_merge(
            $fields[$data['field']]['timestamp_fromto'],
            $fields[$data['field']]['string_fromto']);
          $key = $data['queryname'];
          $old = $node->$key;
          if ($node->$key) {
            $values = explode('|', $node->$key);
          }
          // Convert the table.field format in the fromto fields
          // to the table_field format used by the Views formatters.
          $node->{str_replace('.', '_', $fromto[0])} = $values[0];
          $node->{str_replace('.', '_', $fromto[1])} = $values[1];
          $node->calendar_start = $values[0];
          $node->calendar_end = !empty($values[1]) ? $values[1] : $values[0];
          $items[$delta] = $node;
        }

        foreach ($view->field as $field) {

          // Get the themed label for the field from a Views theme.
          if (!in_array($field['field'], $field_names) && $field['field'] != 'title') {
            if ($view_fields[$field['id']]['visible'] !== FALSE) {
              $node->fields[$field['queryname']] = views_theme_field('views_handle_field', $field['queryname'], $view_fields, $field, $node, $view);
            }
          }
        }

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
          $node->calendar_start_date = date_make_date($node->calendar_start, date_default_timezone_name());
          $node->calendar_end_date   = date_make_date($node->calendar_end, date_default_timezone_name());
          $node->calendar_start = date_format($node->calendar_start_date, DATE_FORMAT_DATETIME);
          $node->calendar_end = date_format($node->calendar_end_date, DATE_FORMAT_DATETIME);
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
  return $nodes;
}

/**
 *  Function to construct back and next navigation from views arguments
 */
function calendar_nav($view, $mini = FALSE, $with_weekno = FALSE) {
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
  // for the mini calendar in a block, treat the url as a
  // querystring to avoid actually changing the page
  if ($mini && $view->calendar_type == 'month') {
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
  $next_date = drupal_clone($prev_date);
  date_modify($next_date, '+2 '. $view->calendar_type);

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
      if ($type != 'week') {
        $nextpath[$x] = $type == 'year' && isset($next_year) ? $next_year : date_format($next_date, $formats[$type]);
        $prevpath[$x] = $type == 'year' && isset($prev_year) ? $prev_year : date_format($prev_date, $formats[$type]);
      }
      else {
        if (date_format($prev_date, 'Y') < $view->year) {
          // If pervious date is in previous year,
          // look up the number of weeks in that year.
          $prev_year = date_format($prev_date, 'Y');
          $prev_template = calendar_year_template($prev_year);
          $prev_week = db_result(db_query("SELECT weeks FROM {calendar_years} WHERE template = %d", $prev_template));
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
 *
 * No good way to do this using PHP date functions which can
 * only identify the ISO week -- get the calendar week info from the
 * calendar_templates table.
 */
function calendar_week_range(&$view) {
  if (isset($view->week)) {
    $template = calendar_year_template($view->year);
    $row = db_fetch_array(db_query("SELECT * FROM {calendar_templates} WHERE template = %d AND weekno = %d", $template, $view->week));
    $min_date = date_make_date($view->year .'-'. $row['start'] .' 00:00:00', date_default_timezone_name());
    $max_date = date_make_date($view->year .'-'. $row['end'] .' 23:59:59', date_default_timezone_name());
    $view->min_date = $min_date;
    $view->max_date = $max_date;
    return array($min_date, $max_date);
  }
  return array();
}