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
 *     Node objects must have GMT timestamp values for start ($node->event_start).
 *     Optionally, an end value ($node->event_end) and a time zone offset value
 *     in the same format as drupal core ($node->tz). If a node has no end
 *     value, it is rendered on only one day. If no time zone value is displayed
 *     the time is rendered with no time zone offset (GMT).
 *  @param $module - String containing the name of the module calling the function
 *  @param $title - A string value that will be printed into the header of the calendar
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
 *                              used in case node array has values that overlap periods
 *  @return Themed calendar view of nodes or array of calendar rows
 */
function calendar_get_calendar($view, $nodes, $module, $title = NULL, $params = array()) {

  $today = calendar_user_date();

  if ($params['limit'][0]) {
    $min_date = $params['limit'][0];
    $max_date = $params['limit'][1];
  }
  else {
    $min_date = 0;
    $max_date = 999999999999;
  }

  $data  = array();
  foreach ($nodes as $nid => $node) {

    $node->stripe = $params['stripe'] ? $node->$params['stripe'] : $node->nid;

    $calendar = new stdClass();
    $nodes[$node->nid] = $node;
    $calendar->nid = $node->nid;

    // $node_start and $node_end are local timestamp values
    if ($min_date -  $node->start_offset > $node->calendar_start) {
      $node->calendar_start = $min_date -  $node->start_offset;
    }
    if ($max_date -  $node->end_offset  < $node->calendar_end) {
      $node->calendar_end = $max_date -  $node->end_offset;
    }

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
      $calendar->stripe = $node->stripe;
      $data[gmdate('Y', $node_start)][gmdate('m', $node_start)][gmdate('j', $node_start)][] = $calendar;
    }
    else {
      // roll through each day the calendar occurs and set an entry for each
      for ($x = $node_start; $x <= $node_end; $x += 86400) {
        if ($x == $node_end) {
          $calendar = new StdClass();
          $calendar->nid = $node->nid;
          $calendar->stripe = $node->stripe;
          $calendar->calendar_state = 'end';
          $calendar->stamp = $x;
          $data[gmdate('Y', $x)][gmdate('m', $x)][gmdate('j', $x)][] = $calendar;
        }
        elseif ($x == $node_start) {
          $calendar = new StdClass();
          $calendar->nid = $node->nid;
          $calendar->stripe = $node->stripe;
          $calendar->calendar_state = 'start';
          $calendar->stamp = $x;
          $data[gmdate('Y', $x)][gmdate('m', $x)][gmdate('j', $x)][] = $calendar;
        }
        else {
          $calendar = new StdClass();
          $calendar->nid = $node->nid;
          $calendar->stripe = $node->stripe;
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
            $content = theme('calendar_date_box', $year, $month, $day, 'table', $params['mini'], $calendars, $params['url']);
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
            if (!$params['hide_header']) {
              $rows[][] = array(
                'class' => 'heading month',
                'id' => 'month'. $month,
                'data' => $month_name,
                'colspan' => $colspan);
            }
            $rows[] = calendar_week_header($params['mini'], $params['with_weekno']);

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
                $content = theme('calendar_date_box', $year, $month, $cur_day, $view, $params['mini'], $selected, $params['url']);

                // render nodes for the day
                if(is_array($days[$cur_day]) && !$params['mini']) {
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
                  'id' => strtolower($month_name . $cur_day),
                  'data' => $params['mini'] ? $content : '<div class="inner">'. $content .'</div>');

                $curstamp += 86400;
                if ($curstamp > $lastday) {
                  $x = 8;
                }
              }
              $cur_week = date('W', $curstamp);

              // print week unless this is a week view and force_week is set and this is not the forced week
              if (!($view == 'week' && $params['force_week'] && $params['force_week'] != date('W', $curstamp))) {
                if ($params['with_weekno']) {
                  $week_row = array(0 =>
                    array(
                      'class' => 'week',
                      'data' => l(sprintf('%02d', $cur_week), $params['url'] .'/'. $year .'/W'. $cur_week),
                    ));
                  $rows[] = array_merge($week_row, array_pad($row, 7, '&nbsp;'));
                }
                else {
                  $rows[] = array_pad($row, 7, '&nbsp;');
                }
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
  $header = $title ? array(array('class' => 'heading', 'data' => $title, 'colspan' => $colspan)) : array();

  $op = $params['mini'] ? 'mini' : 'page';
  return theme('calendar_'. $view, $op, $header, $rows, $month_rows);
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
  global $user;
  static $date;
  if (!$date) {
    $now = calendar_user_time();

    if (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone)) {
      $user_offset = $user->timezone;
    }
    else {
      $user_offset = variable_get('date_default_timezone', 0);
    }

    $now += $user_offset;
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
  case ('week'):
    return gmdate('W', $date);
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
function calendar_week_header($mini = FALSE, $with_week = TRUE) {
  // create week header
  $days = calendar_week_days();

  if ($with_week) {
    $row[] = array('class' => "days week", 'data' => '&nbsp;');
  }
  foreach ($days as $day) {
    $row[] = array('class' => strtolower("days ". $day['day']), 'data' => $mini ? drupal_substr($day['t'], 0 , 1) : $day['t']);
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