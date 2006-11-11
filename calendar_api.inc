<?php
// $Id$
/**
 * Adapted from event_get_calendar() function in the event module
 * Reworked to remove dependency on event module
 *
 * Returns a calendar in the requested format, populated with the provided nodes.
 * This is not used internally, rather it is an API funciton for external modules
 * to use for rendering calendars when constructing thier own event objects.
 *
 * @ingroup event_view
 *
 * @param $view - The format of calendar to return. Possible values:
 *                "table": A tabular calendar.
 *                "month": A month calendar.
 *                "week": A week calendar.
 *                "day": A day calendar.
 *  @param $nodes An associative array of nodes with nids for key values.
 *           Node objects must have GMT timestamp values for start ($node->event_start).
 *           Optionally, an end value ($node->event_end) and a time zone offset value
 *           in the same format as drupal core ($node->tz). If a node has no end
 *           value, it is rendered on only one day. If no time zone value is displayed
 *           the time is rendered with no time zone offset (GMT).
 *  @param $module - String containing the name of the module calling the function
 *  @param $title - A string value that will be printed into the header of the calendar
 *  @param $params - an optional array of additional values that may be needed
 *           $params[force_week]  - a week number to limit the displayed calendar to only that week
 *           $params[hide_header] - true to omit td header row(s)
 *           $params[as_array]    - true to return the rows array instead of the themed table
 *           $params[mini]        - this is a mini calendar, don't display item details
 *           $params[url]         - an url to use as a base url in month name
 *  @return Themed calendar view of nodes or array of calendar rows
 */
function calendar_get_calendar($view, $nodes, $module, $title = NULL, $params = array()) {
  
  $today = calendar_user_date();
  $data  = array();
  foreach ($nodes as $node) {

    $calendar = new stdClass();
    $nodes[$node->nid] = $node;
    $calendar->nid = $node->nid;

    // $node_start and $node_end are local timestamp values
    $node_start = gmmktime(0, 0, 0, calendar_event_date('m', $node->calendar_start, $node->start_offset), calendar_event_date('d', $node->calendar_start, $node->start_offset), calendar_event_date('Y', $node->calendar_start, $node->start_offset));
    if ($node->calendar_end) {
      $node_end = gmmktime(0, 0, 0, calendar_event_date('m', $node->calendar_end, $node->end_offset), calendar_event_date('d', $node->calendar_end, $node->end_offset), calendar_event_date('Y', $node->calendar_end, $node->end_offset));
    }
    else {
      $node_end = $node_start;
    }

    if ($node_start == $node_end) {
      $calendar->calendar_state = 'singleday';
      $calendar->stamp = $node_start;
      $data[gmdate('Y', $node_start)][gmdate('m', $node_start)][gmdate('j', $node_start)][] = $calendar;
    }
    else {
      // roll through each day the calendar occurs and set an entry for each
      for ($x = $node_start; $x <= $node_end; $x += 86400) {
        if ($x == $node_end) {
          $calendar->calendar_state = 'end';
          $calendar->stamp = $x;
          $data[gmdate('Y', $x)][gmdate('m', $x)][gmdate('j', $x)][] = $calendar;
        }
        elseif ($x == $node_start) {
          $calendar->calendar_state = 'start';
          $calendar->stamp = $x;
          $data[gmdate('Y', $x)][gmdate('m', $x)][gmdate('j', $x)][] = $calendar;
        }
        else {
          $calendar->calendar_state = 'ongoing';
          $calendar->stamp = $x;
          $data[gmdate('Y', $x)][gmdate('m', $x)][gmdate('j', $x)][] = $calendar;
        }
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
  
  $weekdays = calendar_week_days();
  
  switch ($view) {
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
            $content = theme('calendar_date_box', $year, $month, $day, 'table', $params['mini'], $calendars);
            foreach($calendars as $calendar) {
              if(!$month_name) {
                $month_name = gmdate('M', $calendar->stamp);
                $dow = _calendar_day_of_week($calendar->stamp);
              }
              $node = $nodes[$calendar->nid];
              
              $node->calendar_state = $calendar->calendar_state;
              if($output = module_invoke($module, 'calendar_node_'. $view, $node)) {
                $content .= $output;
              }
              else {
                $content .= theme('calendar_node_'. $view, $node);
              }
            }
            $rows[][] = array(
              'class' => strtolower("$month_name ". $weekdays[$dow]['day'] . ($calendar->stamp == $today ? ' today' : '')),
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
      $colspan = '7';
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
            // timestamp of first day in month
            $curstamp = gmmktime(0, 0, 0, $month, 1, $year);
            // timestamp of last day in month
            $lastday = gmmktime(0, 0, 0, $month, gmdate('t', $curstamp), $year);
            // pad the first week row array to fill up days in the previous month we don't build
            $row = array_fill(0, 6, '');
            // get the day of week offset value for the first day of the month
            $start = $offset = _calendar_day_of_week($curstamp);
            // get name of month
            $month_name = gmdate('M', $curstamp);
            $month_link = $params['url'] ? l($month_name, $params['url'] .'/'. $year .'/'. intval($month)) : $month_name;
            // set week counter
            $week = 0;
            if (($view == 'month' || !$params['force_week'])) {
              // add month header, skip if a single week is being displayed (the week header gets added later)
              if (!$params['hide_header']) {
                $rows[][] = array(
                  'class' => 'heading month',
                  'id' => 'month'. $month,
                  'data' => $month_name,
                  'colspan' => $colspan);
              }
              $rows[] = calendar_week_header($params['mini']);
            }

            while ($curstamp <= $lastday) {
              for ($x = $start; $x < 7; $x++) {
                $cur_day = (($week * 7) + ($x + 1) - $offset);
                $selected = FALSE;
                if(is_array($days[$cur_day])) {
                  // make sure dummy nodes added to create blank calendars don't get marked as having content
                  foreach ($items = $days[$cur_day] as $item) {
                    if ($item->nid) {
                      $selected = TRUE;
                    }
                  }
                }
                $content = theme('calendar_date_box', $year, $month, $cur_day, $view, $params['mini'], $selected);
                
                // render nodes for the day
                if(is_array($days[$cur_day])) {
                  foreach($days[$cur_day] as $calendar) {
                    $node = $nodes[$calendar->nid];
                    $node->calendar_state = $calendar->calendar_state;
                    if($output = module_invoke($module, 'calendar_node_'. $view, $node)) {
                      $content .= $output;
                    }
                    elseif (!$params['mini'] && $node->nid > 0) {
                      $content .= theme('calendar_node_'. $view, $node);
                    }
                  }
                }

                $row[$x] = array(
                  'class' => strtolower("$month_name ". $weekdays[$x]['day'] . ($curstamp == $today ? ' today' : '') . ($params['mini'] ? ' mini' : '')),
                  'id' => strtolower($month_name . $day),
                  'data' => $content);

                $curstamp += 86400;
                if ($curstamp > $lastday) {
                  $x = 8;
                }
              }
              if ($view == 'week' && $params['force_week'] && $params['force_week'] == date('W', $curstamp)) {
                // add week header if a single week display is desired and this is the week to display
                $cur_week = date('W', $curstamp);
                if (!$params['hide_header']) {
                  $rows[][] = array(
                    'class' => 'heading week',
                    'id' => 'week'. $cur_week,
                    'data' => t('Week ') . $cur_week,
                    'colspan' => $colspan);
                }
                $rows[] = calendar_week_header($params['mini']);
              }
              // print week unless this is a week view and force_week is set and this is not the forced week
              if (!($view == 'week' && $params['force_week'] && $params['force_week'] != date('W', $curstamp))) {
                $rows[] = array_pad($row, 7, '<nobr></nobr>');
              }

              $week++;
              $start = 0;
              $row = array();
            }
            if ($view == 'year' && !$params['as_array']) {
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
  $header[] = ($title ? array('class' => 'heading', 'data' => $title, 'colspan' => $colspan) : array());
  return theme('calendar_'. $view, 'page', $header, $rows, $month_rows);
}

/**
 * Formats local time values to GMT timestamp using time zone offset supplied.
 * All time values in the database are GMT and translated here prior to insertion.
 *
 * Time zone settings are applied in the following order:
 * 1. If supplied, time zone offset is applied
 * 2. If user time zones are enabled, user time zone offset is applied
 * 3. If neither 1 nor 2 apply, the site time zone offset is applied
 *
 * @param $format The date() format to apply to the timestamp.
 * @param $timestamp The GMT timestamp value.
 * @param $offset Time zone offset to apply to the timestamp.
 * @ingroup event_support
 * @return gmdate() formatted date value
 */
function calendar_mktime($hour, $minute, $second, $month, $day, $year, $offset = NULL) {
  global $user;
  $timestamp = gmmktime($hour, $minute, $second, $month, $day, $year);
  if (isset($offset)) {
    return $timestamp - $offset;
  }
  elseif (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone) && (variable_get('event_timezone_display', 'event') == 'user')) {
    return $timestamp - $user->timezone;
  }
  else {
    return $timestamp - variable_get('date_default_timezone', 0);
  }
}

/**
 * Returns a local timestamp based on the user or site time zone.
 * @return integer timestamp
 */
function calendar_user_time() {
  global $user;

  if (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone)) {
    return (time() - date("Z")) + $user->timezone;
  }
  else {
    return (time() - date("Z")) + variable_get('date_default_timezone', 0);
  }
}

/**
 * Returns a local timestamp (as defined by the user or site's timezone) for
 * midnight GMT.
 * @return integer timestamp
 */
function calendar_user_date($part = 'timestamp') {
  static $date;
  if (!$date) {
    $now = calendar_user_time();
    $date = gmmktime(0, 0, 0, gmdate('m', $now), gmdate('j', $now), gmdate('Y', $now));
  }
  switch ($part) {
  case ('year'):
    return gmdate('Y', $date);
  case ('month'):
    return gmdate('m', $date);
  case ('day'):
    return gmdate('j', $date);
  case ('hour'):
    return gmdate('H', $date);
  case ('minute'):
    return gmdate('i', $date);
  default:
    return $date;
  }
}

/**
 * Formats a GMT timestamp to local date values using time zone offset supplied.
 * All timestamp values in event nodes are GMT and translated for display here.
 *
 * Time zone settings are applied in the following order
 * 1. If supplied, time zone offset is applied
 * 2. If user time zones are enabled, user time zone offset is applied
 * 3. If neither 1 nor 2 apply, the site time zone offset is applied
 *
 * @param $format The date() format to apply to the timestamp.
 * @param $timestamp The GMT timestamp value.
 * @param $offset Time zone offset to apply to the timestamp.
 * @ingroup event_support
 * @return gmdate() formatted date value
 */
function calendar_event_date($format, $timestamp, $offset = null) {
  global $user;

  if (isset($offset)) {
    $timestamp += $offset;
  }
  elseif (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone)) {
    $timestamp += $user->timezone;
  }
  else {
    $timestamp += variable_get('date_default_timezone', 0);
  }

  // make sure we apply the site first day of the week setting for dow requests
  if ($format == 'w') {
    $result = calendar_day_of_week($timestamp);
  }
  else {
    $result = gmdate($format, $timestamp);
  }
  return  $result;
}

/**
 * Returns week day names and thier translated values, corrected for the start of week day settings (mon or sun)
 *
 * @ingroup event_support
 * @return an associative array containing weekday names
 */
function calendar_week_days() {
  static $weekdays;

  if (!$weekdays) {
    if (variable_get('date_first_day', 1)) {
      $weekdays = array(array('day' => 'Mon', 't' => t('Mon')), array('day' => 'Tue', 't' => t('Tue')), array('day' => 'Wed', 't' => t('Wed')), array('day' => 'Thu', 't' => t('Thu')), array('day' => 'Fri', 't' => t('Fri')), array('day' => 'Sat', 't' => t('Sat')), array('day' => 'Sun', 't' => t('Sun')));
    }
    else {
      $weekdays = array(array('day' => 'Sun', 't' => t('Sun')), array('day' => 'Mon', 't' => t('Mon')), array('day' => 'Tue', 't' => t('Tue')), array('day' => 'Wed', 't' => t('Wed')), array('day' => 'Thu', 't' => t('Thu')), array('day' => 'Fri', 't' => t('Fri')), array('day' => 'Sat', 't' => t('Sat')));
    }
  }
  return $weekdays;
}
/**
 * Formats the weekday information into table header format
 *
 * @ingroup event_support
 * @return array with weekday table header data
 */
function calendar_week_header($mini = FALSE) {
  // create week header
  $days = calendar_week_days();

  foreach ($days as $day) {
    $row[] = array('class' => strtolower("days ". $day['day']), 'data' => $mini ? substr($day['t'], 0 , 1) : $day['t']);
  }

  return $row;
}
/**
 * Return the day of week with start of week offset applied
 * @param $stamp GMT timestamp
 * @return integer day of the week
 */
function _calendar_day_of_week($stamp) {
  $dow = gmdate('w', $stamp);
  $dow = (variable_get('date_first_day', 1) ? ($dow == 0 ? 6 : --$dow ) : $dow);
  return $dow;
}

/**
 * @addtogroup themeable
 * @{
 */

/**
 * Format a calendar view
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_year($op, $header, $rows, $month_rows) {
  $year = array_shift($month_rows);
  $output = '<div class="calendar-calendar"><div class="year-view">';
  $output .= $year;
    foreach ($month_rows as $month) {
      $output .= '<div class="mini">'.$month.'</div>';    
    }
  $output .= "</div></div>\n";
  return $output;
}

/**
 * Format a calendar view
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_month($op, $header, $rows) {
  if (strstr($header[0]['data'], '<table')) {
    $output = $header[0]['data'];
    $header = array();
  }
  $output .= theme("table", $header, $rows);
  return '<div class="calendar-calendar"><div class="month-view">'. $output ."</div></div>\n";
}

/**
 * Format a calendar view
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_week($op, $header, $rows) {
  $output = theme("table", $header, $rows);
  return '<div class="calendar-calendar"><div class="week-view">'. $output ."</div></div>\n";
}

/**
 * Format a calendar view
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_day($op, $header, $rows) {
  if (strstr($header[0]['data'], '<table')) {
    $output = $header[0]['data'];
    $header = array();
  }
  $output .= theme("table", $header, $rows);
  return '<div class="calendar-calendar"><div class="day-view">'. $output ."</div></div>\n";
}

/**
 * Format a calendar view
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_table($op, $header, $rows) {
  $output = theme("table", $header, $rows);
  return '<div class="calendar-calendar"><div class="table-view">'. $output ."</div></div>\n";
}

/**
 * Format a calendar view
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_list($op, $header, $rows) {
  return '<div class="calendar-calendar"><div class="list-view">'. $rows ."</div></div>\n";
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */
function theme_calendar_node_day($node) {
  $output .= '<div class="calendar dayview">'."\n";

  $output .= '<div class="title">'. l($node->title, "node/$node->nid", array('title' => t('view this calendar'))) .'</div>'."\n";

  $output .= '<div class="start">'. t('Start: ') . $node->start_time_format .'</div>'."\n";
  if ($node->calendar_start != $node->calendar_end) {
    $output .= '<div class="end">'. t('End: ') . $node->end_time_format .'</div>'."\n";
  }
  if ($node->teaser) {
    $output .= '<div class="content">'. check_markup($node->teaser) ."</div>\n";
  }
  $output .= '<div class="links">'. theme('links', $node->calendar_links) ."\n</div>";
  $output .= "</div>\n";
  return $output;
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */
function theme_calendar_node_week($node) {
  static $stripe, $stripe_map;

  if(!$stripe_map[$node->nid]) {
    if($stripe >= 10) {
      $stripe = 1;
    }
    else {
      $stripe++;
    }
    $stripe_map[$node->nid] = $stripe;
  }

  $output .= '<div class="calendar weekview">'."\n";
  $output .= '<div class="stripe-'. $stripe_map[$node->nid] .'"></div>'."\n";

  switch ($node->calendar_state) {
    case 'singleday':
        $times = '<div class="start">'. t('Start: ') . $node->start_time_format .'</div>'."\n";
        if ($node->calendar_start != $node->calendar_end) {
          $times .= '<div class="end">'. t('End: ') . $node->end_time_format .'</div>'."\n";
        }
      break;
    case 'start':
        $times = '<div class="start">'. t('Start: ') . $node->start_time_format .'</div>'."\n";
      break;
    case 'end':
        $times = '<div class="end">'. t('End: ') . $node->end_time_format .'</div>'."\n";
      break;
    case 'ongoing':
        $times = '<div class="ongoing">'. t('all day') .'</div>'."\n";
      break;
  }

  if (variable_get('calendar_type_control', 'all') != 'never') {
    if ((calendar_get_types('all') + calendar_get_types('solo')) > 1) {
      $output .= '<div class="type">'. l('('. $node->calendar_node_title .')', 'calendar/'. gmdate('Y/m/d', $node->calendar_current_date) .'/month/'.$node->type, array('title' => t('limit view to calendars of this type'))).'</div>'."\n";
    }
  }
  $output .= '<div class="title">'. l($node->title, "node/$node->nid", array('title' => t('view this calendar'))) .'</div>'."\n";
  $output .= $times;
  $output .= '<div class="links">'. theme('links', $node->calendar_links) ."\n</div>";
  $output .= '</div>' . "\n";
  return $output;
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */
function theme_calendar_node_month($node) {
  static $stripe, $stripe_map;
  if (!$stripe_map[$node->nid]) {
    if ($stripe >= 10) {
      $stripe = 1;
    }
    else {
      $stripe++;
    }
    $stripe_map[$node->nid] = $stripe;
  }

  $output .= '<div class="calendar monthview">'."\n";
  $output .= '<div class="stripe-'. $stripe_map[$node->nid] .'"></div>'."\n";

  switch ($node->calendar_state) {
    case 'singleday':
      if ($node->calendar_start_time_format != $node->calendar_end_time_format) {
        $times = '<div class="start">'. t('Start: ') . $node->start_time_format .'</div>'."\n";
        if ($node->calendar_start != $node->calendar_end) {
          $times .= '<div class="end">'. t('End: ') . $node->end_time_format .'</div>'."\n";
        }
      }
      else {
        $times = '<div class="start">'. $node->start_time_format .'</div>'."\n";
      }
      break;
    case 'start':
        $times = '<div class="start">'. t('Start: ') . $node->start_time_format .'</div>'."\n";
      break;
    case 'end':
        $times = '<div class="end">'. t('End: ') . $node->end_time_format .'</div>'."\n";
      break;
    case 'ongoing':
        $times = '<div class="ongoing">'. t('all day') .'</div>'."\n";
      break;
  }
  $output .= '<div class="title">'. l($node->title, "node/$node->nid", array('title' => t('view this calendar'))) .'</div>'."\n";
  $output .= $times;
  $output .= '<div class="links">'. theme('links', $node->calendar_links) ."\n</div>";
  $output .= '</div>' . "\n";

  return $output;
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */

function theme_calendar_node_table($node) {
  static $stripe, $stripe_map, $link_count;
  drupal_add_js(drupal_get_path('module', 'calendar') .'/calendar.js');
  $link_count++;

  if (!$stripe_map[$node->nid]) {
    if ($stripe >= 10) {
      $stripe = 1;
    }
    else {
      $stripe++;
    }
    $stripe_map[$node->nid] = $stripe;
  }

  $output .= '<div class="calendar tableview">'."\n";
  $output .= '<div class="stripe-'. $stripe_map[$node->nid] .'"></div>'."\n";
  if (variable_get('calendar_type_control', 'all') != 'never') {
    if (!$module && (calendar_get_types('all') + calendar_get_types('solo')) > 1) {
      $output .= '<div class="type">'. l('('. $node->calendar_node_title .')', 'calendar/'.gmdate('Y/m/d', $node->calendar_start).'/table/'.$node->type, array('title' => t('limit view to calendars of this type'))).'</div>'."\n";
    }
  }
  $output .= '<div class="title">'. l($node->title, "node/$node->nid", array('title' => t('view this calendar'))) .'</div>'."\n";

  switch ($node->calendar_state) {
    case 'singleday':
        $output .= '<div class="start">'. t('Start: ') . $node->start_time_format ."</div>\n";
        if ($node->calendar_start != $node->calendar_end) {
          $output .= '<div class="end">'. t('End: ') . $node->end_time_format ."</div>\n";
        }
      break;
    case 'start':
        $output .= '<div class="start">'. t('Start: ') . $node->start_time_format ."</div>\n";
      break;
    case 'end':
        $output .= '<div class="end">'. t('End: ') . $node->end_time_format ."</div>\n";
      break;
    case 'ongoing':
        $output .= '<div class="ongoing">('. t('all day') .')</div>'."\n";
      break;
  }

  $output .= '<div id="info_'. $link_count .'" class="info">'."\n";
  if ($node->calendar_state != 'singleday') {
    $output .= '<div class="start">'. t('Start: ') . $node->start_format .'</div>'."\n";
    if ($node->calendar_start != $node->calendar_end) {
      $output .= '<div class="end">'. t('End: ') . $node->end_format .'</div>'."\n";
    }
  }
  $output .= '<div class="content">'. check_markup($node->teaser) .'</div></div>'."\n";

  $links = $node->calendar_links;
  $links[] = l(t('more info'), 'node/'.$node->nid, array('onclick' => "popupnotes('info_". $link_count ."'); return false;", 'title' => t('Show detailed information for this calendar.')));
  $output .= '<div class="links">'. theme('links', $links) ."\n</div>";
  $output .= '</div>'."\n";
  return $output;
}

function theme_calendar_node_list($node, $module = NULL) {
  static $stripe, $stripe_map, $link_count;
  drupal_add_js(drupal_get_path('module', 'calendar') .'/calendar.js');
  $link_count++;

  if (!$stripe_map[$node->nid]) {
    if ($stripe >= 10) {
      $stripe = 1;
    }
    else {
      $stripe++;
    }
    $stripe_map[$node->nid] = $stripe;
  }

  $output = '<div class="calendar tableview">'."\n";
  $output .= '<div class="stripe-'. $stripe_map[$node->nid] .'"></div>'."\n";
  if (variable_get('calendar_type_control', 'all') != 'never') {
    if (!$module && (calendar_get_types('all') + calendar_get_types('solo')) > 1) {
      $output .= '<div class="type">'. l('('. $node->calendar_node_title .')', 'calendar/'.gmdate('Y/m/d', $node->calendar_start).'/table/'.$node->type, array('title' => t('limit view to calendars of this type'))).'</div>'."\n";
    }
  }
  $output .= '<div class="title">'. l($node->title, "node/$node->nid", array('title' => t('view this calendar'))) .'</div>'."\n";

  switch ($node->calendar_state) {
    case 'singleday':
        $output .= '<div class="start">'. t('Start') .': '. $node->start_time_format ."</div>\n";
        if ($node->calendar_start != $node->calendar_end) {
          $output .= '<div class="end">'. t('End') .': '. $node->end_time_format ."</div>\n";
        }
      break;
    case 'start':
        $output .= '<div class="start">'. t('Start') .': '. $node->start_time_format ."</div>\n";
      break;
    case 'end':
        $output .= '<div class="end">'. t('End') .': '. $node->end_time_format ."</div>\n";
      break;
    case 'ongoing':
        $output .= '<div class="ongoing">('. t('all day') .')</div>'."\n";
      break;
  }

  $output .= '<div id="info_'. $link_count .'" class="info">'."\n";
  if ($node->calendar_state != 'singleday') {
    $output .= '<div class="start">'. t('Start') .': '. $node->start_format .'</div>'."\n";
    if ($node->calendar_start != $node->calendar_end) {
      $output .= '<div class="end">'. t('End') .': '. $node->end_format .'</div>'."\n";
    }
  }
  $output .= '<div class="content">'. check_markup($node->teaser) .'</div></div>'."\n";

  $links = $node->calendar_links;
  $links[] = l(t('more info'), 'node/'.$node->nid, array('onclick' => "popupnotes('info_". $link_count ."'); return false;", 'title' => t('Show detailed information for this calendar.')));
  $output .= '<div class="links">'. theme('links', $links) ."\n</div>";
  $output .= '</div>'."\n";
  return $output;
}

/**
 * Format an date's day box in a calendar
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_date_box($year, $month, $day, $view, $mini = FALSE, $selected = FALSE) {
  $url = 'calendar/'. $year .'/'. $month .'/'. $day;
  if ($mini) {
    if ($selected) {
      return '<div class="mini_day_on">'. l($day, $url) .'</div>';
    }
    else {
      return '<div class="mini_day_off">'. l($day, $url) .'</div>';
    }
  }
  switch ($view) {
    case 'table':
        $output = '<div class="day">'. l(t('%month / %day', array('%month' => $month, '%day' => $day)), $url) .'</div>'."\n";
      break;
    case 'list':
        $output = '<div class="day">'. l(format_date(_calendar_mktime(0, 0, 0, $month, $day, $year), 'custom', 'l, F j, Y'), $url) .'</div>'."\n";
      break;
    case 'day':
     break;
    default:
        $output = '<div class="day">'. l($day, $url) .'</div>'."\n";
      break;
  }
  return $output;
}

/**
 * Format an empty day on a calendar
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_empty_day($year, $month, $day, $view) {
  switch ($view) {
    case 'table':
        $output = '<div class="day">'. t('%month / %day', array('%month' => $month, '%day' => $day)) .'</div>'."\n";
        $output .= '<div class="calendar-empty"></div>'."\n";
      break;
    case 'day':
    case 'list':
     break;
    default:
        $output = '<div class="day">'. $day .'</div>'."\n";
        $output .= '<div class="calendar-empty"></div>'."\n";
      break;
  }
  return $output;
}

/**
 * Format an date value for a nodeapi insert
 *
 * @param node
 *   The node which needs it's dates formatted
 */
function theme_calendar_nodeapi($node) {
  $output = '<div class="calendar-nodeapi"><div class="'. $node->type. '-start"><label>'. t('Start: ') .'</label>'. $node->start_format .'</div></div>'."\n";
  if ($node->calendar_start != $node->calendar_end) {
    $output .= '<div class="calendar-nodeapi"><div class="'. $node->type. '-end"><label>'. t('End: ') .'</label>'. $node->end_format .'</div></div>'."\n";
  }
  if (variable_get('configurable_timezones', 1)) {
    include_once(EVENT_PATH .'/calendar_timezones.inc');
    $zones = calendar_zonelist();
    $output .= '<div class="calendar-nodeapi"><div class="'. $node->type. '-tz"><label>'. t('Timezone: ') .'</label>'. $zones[$node->timezone] .'</div></div>'."\n";
  }
  return $output;
}

/**
 * Format the calendar filter control dropdown
 *
 * @param form
 *   The form containing the taxonomy controls
 */
function theme_calendar_filter_control($form) {
  return '<div class="calendar-filter-control">'. $form .'</div>';
}

/**
 * Format the 'next' navigation controls for calendar calendars
 *
 * @param link
 *   The url for the navigation
 */
function theme_calendar_nav_next($url, $text = TRUE) {
  return '<span class="next">'. l(($text ? t('next') : '') .' »', $url) .'</span>';
}

/**
 * Format the 'previous' navigation controls for calendar calendars
 *
 * @param link
 *   The url for the navigation
 */
function theme_calendar_nav_prev($url, $text = TRUE) {
  return '<span class="prev">'. l('« '. ($text ? t('prev') : ''), $url) .'</span>';
}

/**
 * Format the links for calendar calendars
 *
 * @param links
 *   An array of links to render
 * @param view
 *   The current view being rendered
 */
function theme_calendar_links($links, $view) {
  return '<p>» '. theme('links', $links) .'</p>';
}

/**
 * Format the ical link
 *
 * @param path
 *   The url for the ical feed
 */
function theme_calendar_ical_link($path) {
  $ical_link = '<div class="ical-link">'. l('<img src="'. base_path() . drupal_get_path('module', 'calendar') .'/images/ical16x16.gif" alt="'.t('Add to iCalendar').'" />', $path, array('title' => t('Add this calendar to your iCalendar')), NULL, NULL, TRUE, TRUE) .'</div>';
  return preg_replace("|http://|","webcal://", $ical_link);
}

/**
 * Format the 'read more' link for calendars
 *
 * @param path
 *   The url to use for the read more link
 */
function theme_calendar_more_link($path) {
  return '<div class="more-link">'. l(t('more'), $path, array('title' => t('More calendars.'))) .'</div>';
}

/**
 * Format an individual upcoming calendar block item
 *
 * @param node
 *   The node to render as an upcoming calendar
 */
function theme_calendar_upcoming_item($node) {
  $output = l($node->title, "node/$node->nid", array('title' => $node->title));
  $output .= '<span class="calendar-timeleft">('. $node->timeleft .')</span>';
  return $output;
}

/**
 * Format the upcoming calendar block for calendar calendars
 *
 * @param items
 *   An array of themed upcoming calendars
 */
function theme_calendar_upcoming_block($items) {
  $output = theme("item_list", $items);
  return $output;
}
