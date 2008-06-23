<?php
//$Id$
/**
 * Build calendar
 *
 * @param unknown_type $view
 * @param unknown_type $items
 * @return themed table
 */
function calendar_build_calendar($view, $items) {
  // Remove nodes outside the selected date range.
  $values = array();
  foreach ($items as $delta => $item) {
    if (($item->calendar_start_date >= $view->min_date && $item->calendar_start_date <= $view->max_date)
     || ($item->calendar_end_date >= $view->min_date && $item->calendar_end_date <= $view->max_date)) {
      $values[date_format($item->calendar_start_date, 'Y-m-d')][date_format($item->calendar_start_date, 'H')][] = $item;
    }
  }
  $items = $values;
  ksort($items);

  $rows = array();
  $curday = drupal_clone($view->min_date);
  date_timezone_set($curday, timezone_open(date_default_timezone_name()));
  date_time_set($curday, 0, 0, 0);

  switch ($view->granularity) {
    case 'year':
      $rows = array();
      for ($i = 1; $i <= 12; $i++) {
        $rows[$i] = calendar_build_month($curday, $view, $items);
      }
      break;

    case 'month':
      $rows = calendar_build_month($curday, $view, $items);
      break;

    case 'day':
      $rows = calendar_build_day($curday, $view, $items);
      break;

    case 'week':
      $rows = calendar_build_week($curday, $view, $items);

      // Merge the day names in as the first row.
      $rows = array_merge(array(calendar_week_header($view)), $rows);
      break;
  }
  return $rows;
  
}

/**
 * Build one month.
 */
function calendar_build_month(&$curday, $view, $items) {
  $month = date_format($curday, 'n');
  date_modify($curday, '-' . strval(date_format($curday, 'j')-1) . ' days');

  $rows = array();
  do {
    $rows = array_merge($rows, calendar_build_week($curday, $view, $items, TRUE));
  } while (date_format($curday, 'n') == $month && $curday <= $view->max_date);

  // Merge the day names in as the first row.
  $rows = array_merge(array(calendar_week_header($view)), $rows);
  return $rows;
}

/**
 * Build one week row.
 */
function calendar_build_week(&$curday, $view, $items, $check_month = FALSE) {
  $weekdays = calendar_untranslated_days($items, $view);
  $today = date_format(date_now(date_default_timezone_name()), 'Y-m-d');
  $month = date_format($curday, 'n');
  $week = date_week(date_format($curday, 'Y-m-d'));

  $first_day = variable_get('date_first_day', 0);

  // move backwards to the first day of the week
  $day_wday = date_format($curday, 'w');
  date_modify($curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');

  // If we're displaying the week number, add it as the
  // first cell in the week.
  if (!empty($view->with_weekno) && $view->granularity != 'day') {
    $url = $view->get_url() .'/'. $view->year .'-W'. $week;
    $rows[$week][] = array(
      'data' => l($week, $url, array('query' => !empty($view->append) ? $view->append : '')),
      'id' => $view->name . '-weekno-' . date_format($curday, 'Y-m-d'),  
      'class' => 'week');
  }

  for ($i = 0; $i < 7; $i++) {
    if ($check_month && ($curday < $view->min_date || $curday > $view->max_date || date_format($curday, 'n') != $month)) {
      $class = strtolower($weekdays[$i] .
      ($view->mini ? ' mini' : ''));
      $content = theme('calendar_empty_day');
    }
    else {
      $content = calendar_build_day($curday, $view, $items);
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
function calendar_build_day($curday, $view, $items) {
  $nextday = drupal_clone($curday);
  date_modify($nextday, '+1 day');
  $inner = '';
  $selected = FALSE;
  foreach ($items as $date => $day) {
    if ($date == date_format($curday, 'Y-m-d')) {
      $selected = TRUE;
      ksort($day);
      foreach ($day as $hour) {
        foreach ($hour as $item) {
          if (!$view->mini) {
            $theme = isset($item->calendar_node_theme) ? $item->calendar_node_theme : 'calendar_'. $view->granularity .'_node';
            $inner .= theme($theme, $item, $view);
          }
        }
      }
    }
  }
  if (empty($inner)) {
    $inner = theme('calendar_empty_day');
  }
  $content = theme('calendar_datebox', date_format($curday, 'Y-m-d'), $view, $items, $selected) . $inner;
  return $content;
}

/**
 * Formats the weekday information into table header format
 *
 * @ingroup event_support
 * @return array with weekday table header data
 */
function calendar_week_header($view) {
  $len = isset($view->name_size) ? $view->name_size : (!empty($view->mini) ? 1 : 3);
  $with_week = !empty($view->with_weekno);
  
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
    $label = $len < 3 ? drupal_substr($translated_days[$delta], 0 , $len) : $translated_days[$delta];
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
  if (empty($view->min_date) || empty($view->max_date)) {
    return $items;
  }
  $view->nodes_per_page = 0;
  $type_names = node_get_types('names');

  $fields = date_api_fields();
  foreach($fields['name'] as $field) {
    $datefields[] = $field['query_name'];
  }
  $view_fields      = date_api_views_fetch_fields('node', 'field');
  $field_names = (array) array_keys($fields);
  $nodes = array();
  $i = 0;
  foreach ($view->argument['date_argument']->options['date_fields'] as $name) {
    $field = $fields['name'][$name];
    $field_type   = strstr($field['type'], 'string') ? 'string' : 'timestamp';
    $alias        = $field['query_name'];
    $field_name   = $field['field_name'];
    $fromto       = $field['fromto'];
    $tz_handling  = $field['tz_handling'];
    $label        = isset($view->field[$name]) ? $view->field[$name]['label'] : $field['field_name'];
    $tz_alias     = str_replace('.', '_', $field['timezone_field']);
    $db_tz = 'UTC';
    $local_tz = date_default_timezone_name();
    switch ($field['tz_handling']) {
      case 'none':
        $db_tz = date_default_timezone_name();
        break;
      case 'date':
        $local_tz = 'date';
        break;
    }
    
    if (strstr($field['type'], 'cck')) {
      $cck_field_name = str_replace(array('_value2', '_value'), '', $field_name);
      $format = date_formatter_format(isset($data['options']) ? $data['options'] : 'default', $cck_field_name);
    }
    elseif (!empty($view->field[$name]['date_format'])) {
      $format = $view->field[$name]['date_format'];
      switch ($view->field[$name]['date_format']) {
        case 'long':
          $format = variable_get('date_format_long',  'l, F j, Y - H:i');
          break;
        case 'medium':
          $format = variable_get('date_format_medium',  'D, m/d/Y - H:i');
          break;
        case 'custom':
          $format = $view->field[$name]['date_format'];
          break;
        default:
          $format = variable_get('date_format_short', 'm/d/Y - H:i');
          break;
      }
    }
    else {
      $format = variable_get('date_format_short', 'm/d/Y - H:i');
    }
    foreach ($items as $pos => $item) {
      if (isset($item->calendar_fields->$alias)) {
        // Create from and to date values for each item, adjusted to
        // the correct timezone.
        $values[0] = $item->calendar_fields->$fromto[0];
        $values[1] = $item->calendar_fields->$fromto[1];
        
        $to_zone = $local_tz;
        if ($tz_handling == 'date' && !empty($item->$tz_alias)) {
          $to_zone = $item->$tz_alias;
        }
        $date = date_make_date($values[0], $db_tz, $field['sql_type']);
        if ($db_tz != $to_zone) {
          date_timezone_set($date, timezone_open($to_zone));
        }
        $values[0] = date_format($date, DATE_FORMAT_DATETIME);
        $date = date_make_date($values[1], $db_tz, $field['sql_type']);
        if ($db_tz != $to_zone) {
          date_timezone_set($date, timezone_open($to_zone));
        }
        $values[1] = date_format($date, DATE_FORMAT_DATETIME);
        
        // Create a node for each date within the field's date range,
        // limited to the view's date range.
        $now = max(date_format($view->min_date, 'Y-m-d'), substr($values[0], 0, 10));
        $to  = min(date_format($view->max_date, 'Y-m-d'), substr($values[1], 0, 10));
        if (empty($to)) {
          $to = $now;
        }
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
          //$node->url = 'node/'. $node->nid;
          $node->$fromto[0] = $values[0];
          $node->$fromto[1] = $values[1];
          
          // Flag which datefield this node is using, in case
          // there are multiple date fields in the view.
          $node->datefield = $alias;

          // If there are other datefields in the View, get rid
          // of them in this pseudo node. There should only be one
          // date in each calendar node.
          foreach ($node as $key => $val) {
            if ($key != $alias && in_array($key, $datefields)) {
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
          
          unset($node->calendar_fields);
          if (isset($node) && (empty($node->calendar_start))) {
            // if no date for the node and no date in the item
            // there is no way to display it on the calendar
            unset($node);
          }
          else {
            $delta = !empty($field['delta_field']) && !empty($item->{$field['delta_field']}) ? $item->{$field['delta_field']} : 0;
            $real_field = $field_name;
            if (substr($field['type'], 0, 3) == 'cck') {
              $real_field = str_replace(array('_value2', '_value'), '', $field_name);
            }
            $id = 'calendar:'. $item->nid .':'. $real_field .':'. $delta .':'. $pos;
            $node->stripe = calendar_node_stripe($view, $node, $alias, $alias);
            $node->date_id = $id;
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
  
  $all_fields = date_api_views_fetch_fields($base, 'field');
  $fields = array();
  foreach ((array) $all_fields as $name => $val) {
    $fromto = array();
    $tmp = explode('.', $name);
    $field_name = $tmp[1];
    $table_name = $tmp[0];
    $alias = str_replace('.', '_', $name);
      
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

      $related_fields = array();
      $timezone_field = '';
      $delta_field = '';
      $rrule_field = '';
      
      // Handling for content field dates
      if ($handler->content_field['tz_handling']) {
        $tz_handling = $handler->content_field['tz_handling'];
        $db_info = content_database_info($handler->content_field);
        if ($tz_handling == 'date') {
          $offset_field = $table_name .'.'. $db_info['columns']['offset']['column'];
          $timezone_field = $table_name .'.'. $db_info['columns']['timezone']['column'];
          $related_fields = array(
            $table_name .'.'. $field_name,
            $table_name .'.'. $db_info['columns']['value2']['column'],
            $table_name .'.'. $db_info['columns']['timezone']['column'],
            $table_name .'.'. $db_info['columns']['offset']['column'],
            $table_name .'.'. $db_info['columns']['offset2']['column'],
          );
        }
        else {
          $related_fields = array(
            $table_name .'.'. $field_name,
            $table_name .'.'. $db_info['columns']['value2']['column'],
          );
        }
      }
      // Get the delta value into the query.
      if (!empty($handler->content_field['multiple'])) {
        array_push($related_fields, "$table_name.delta");
        $delta_field = 'delta';
      }
      if (!empty($db_info['columns']['rrule'])) {
        $rrule_field = $table_name .'.'. $db_info['columns']['rrule']['column'];
      }
      // Handling for simple timestamp fields
      else {
        $fromto = array($alias, $alias);
        $tz_handling = 'site';
      }
      // Handling for cck fromto dates
      switch ($handler->content_field['type']) {
        case 'date':
        case 'datetime':
        case 'datestamp':
          $db_info = content_database_info($handler->content_field);
          $fromto = array(
            $table_name .'_'. $db_info['columns']['value']['column'],
            $table_name .'_'. ($handler->content_field['todate'] ? $db_info['columns']['value2']['column'] : $db_info['columns']['value']['column']),
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
      $fields['name'][$name] = array(
        'type' => $type,
        'sql_type' => $sql_type,
        'label' => $val['group'] .': '. $val['title'],
        'granularity' => $granularity,
        'fullname' => $name,
        'table_name' => $table_name,
        'field_name' => $field_name,
        'query_name' => $alias,
        'fromto' => $fromto,
        'tz_handling' => $tz_handling,
        'offset_field' => $offset_field,
        'timezone_field' => $timezone_field,
        'delta_field' => $delta_field,
        'rrule_field' => $rrule_field,
        'related_fields' => $related_fields,
      );
      $fields['alias'][$alias] = $fields['name'][$name];
    }
  }
  //cache_set($cid, $fields, 'cache_views');
  return $fields;
}
