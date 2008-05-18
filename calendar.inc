<?php
//$Id$
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
  return $rows;
  
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
  $view_fields      = views_fetch_fields('node', 'field');
  $field_names = (array) array_keys($fields);
  $nodes = array();
  $i = 0;

  foreach ($view->argument['calendar_date']['date_fields'] as $name) {
    $field = $fields[$name];
    $view_field = $view->field[$name];
    $option           = $fields[$data['field_name']];
    $field_type       = strstr($option['type'], 'string') ? 'string' : 'timestamp';
    $field_field      = $field['query_name'];
    $field_field_name = $field['field_name'];
    $timestamp_fromto = $field['timestamp_fromto'];
    $string_fromto    = $field['string_fromto'];
    $tz_handling      = $field['tz_handling'];
    $label            = $view_field['label'];
        
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

    $fromto = array_merge($timestamp_fromto, $string_fromto);

    foreach ($items as $delta => $item) {
      // The query created from/to dates like 2007-12-31T01:30:00|2007-12-31T02:30:00,
      // explode those values and separate them into from and to dates on the node.
      if (isset($item->$field_field)) {
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
          //$node->title = $node->node_title;
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
  return $nodes;
}

/**
 *  Function to construct back and next navigation from views arguments
 */
function calendar_nav(&$view, $with_weekno) {
  $path = array();
  $prev_date = drupal_clone($view->min_date);
  date_modify($prev_date, '-1 '. $view->calendar_type);
  $next_date = drupal_clone($view->min_date);
  date_modify($next_date, '+1 '. $view->calendar_type);

  $format = array('year' => 'Y', 'month' => 'Y-m', 'day' => 'Y-m-d', 'week' => 'Y\Ww');
 
  $current_url = $view->get_url();
  
  switch ($view->calendar_type) {
    case 'week':
      break;
    default:
      $next_path = str_replace($view->calendar_arg, date_format($next_date, $format[$view->calendar_type]), $current_url);
      $prev_path = str_replace($view->calendar_arg, date_format($prev_date, $format[$view->calendar_type]), $current_url);
  }
  
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
  $header[]  = array('data' => theme('calendar_nav_prev', $prev_path, $view->build_type == 'block' ? FALSE : TRUE), 'class' => 'prev', 'colspan' => $colspan_prev);
  $header[]  = array('data' => $view->subtitle, 'class' => 'heading', 'colspan' => $colspan_middle);
  $header[]  = array('data' => theme('calendar_nav_next', $next_path, $view->build_type == 'block' ? FALSE : TRUE), 'class' => 'next', 'colspan' => $colspan_next);

  return $header;
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

/**
 *  Identify all potential date/timestamp fields.
 *
 *  @return
 *   array with fieldname, type, and table
 */
function _calendar_fields($base = 'node') {
  $cid = 'calendar_fields';
  cache_clear_all($cid, 'cache_views');
  
  $all_fields = views_fetch_fields($base, 'field');
  $fields = array();
  foreach ((array) $all_fields as $name => $val) {
    $timestamp_fromto = array();
    $string_fromto = array();
    $tmp = explode('.', $name);
    $field_name = $tmp[1];
    $table_name = $tmp[0];
    
    $handler = views_get_handler($table_name, $field_name, 'field');
    $type = '';
    
    // For cck fields, get the date type.
    if (isset($handler->content_field)) {
      if ($handler->content_field['type'] == 'date') {
        $type = 'cck_string';
      }
      elseif ($handler->content_field['type'] == 'datestamp') {
        $type = 'cck_timestamp';
      }
      elseif ($handler->content_field['type'] == 'datetime') {
        $type = 'cck_datetime';
      }
    }
    
    // This is a core timestamp field.
    elseif (strstr($field_name, 'timestamp') || strstr($field_name, 'updated') 
      || strstr($field_name, 'created') || strstr($field_name, 'changed')) {
      $type = 'timestamp';
    }
    

    // Don't do anything if this is not a date field we can handle.
    if (!empty($type)) {

      // dates with from and to dates need to handle both fields as one
      // add the from and to dates to the first one found and ignore the second
      $fields[$name]['table_name'] = $table_name;
      $fields[$name]['field_name'] = $field_name;
      $fields[$name]['type'] = $type;
    
      // Handling for content field dates
      if ($handler->content_field['tz_handling']) {
        $tz_handling = $handler->content_field['tz_handling'];
        if ($tz_handling == 'date') {
          $offset_field = $table_name .'.'. $handler->content_field['columns']['offset']['column'];
        }
        $related_fields = array(
          $table_name .'.'. $field_name .'_value',
          $table_name .'.'. $field_name .'_value2',
          $table_name .'.'. $field_name .'_timezone',
          $table_name .'.'. $field_name .'_offset',
          $table_name .'.'. $field_name .'_offset2',
          );
        $timezone_field = $table_name .'.'. $handler->content_field['columns']['timezone']['column'];
      }
      // Handling for simple timestamp fields
      else {
        $timestamp_fromto = array($name, $name);
        $tz_handling = 'site';
        $related_fields = array();
        $timezone_field = '';
      }
      // Handling for cck fromto dates
      switch ($handler->content_field['type']) {
        case 'datestamp':
          $timestamp_fromto = array(
            $table_name .'.'. $field_name .'_value',
            $table_name .'.'. ($handler->content_field['todate'] ? $field_name .'_value2' : $field_name .'_value'),
            );
          break;
        case 'date':
          $string_fromto = array(
            $table_name .'.'. $field_name .'_value',
            $table_name .'.'. ($handler->content_field['todate'] ? $field_name .'_value2' : $field_name .'_value'),
            );
          break;
      }
      
      if (is_array($handler->content_field['granularity'])) {
        $granularity = $handler->content_field['granularity'];
      }
      else {
        $granularity = array('year', 'month', 'day', 'hour', 'minute');
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
      $fields[$name] = array(
        'type' => $type,
        'sql_type' => $sql_type,
        'label' => $val['group'] .': '. $val['title'],
        'granularity' => $granularity,
        'fullname' => $name,
        'table_name' => $table_name,
        'field_name' => $field_name,
        'query_name' => str_replace('.', '_', $name),
        'timestamp_fromto' => $timestamp_fromto,
        'string_fromto' => $string_fromto,
        'tz_handling' => $tz_handling,
        'offset_field' => $offset_field,
        'timezone_field' => $timezone_field,
        'related_fields' => $related_fields,
      );
    }
  }
  //cache_set($cid, $fields, 'cache_views');
  return $fields;
}