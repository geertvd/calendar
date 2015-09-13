<?php

/**
 * @file
 * Views style plugin for the Calendar module.
 *
 * The style plugin is responsible for setting global values needed by
 * the row plugin. It then gathers up an array of view results (as restructured
 * by the row plugin) and organizes them into year/month/day/time
 * arrays that the theme can use to display the results in a calendar.
 */

/**
 * Default style plugin to render an iCal feed.
 */
class calendar_plugin_style extends views_plugin_style {


  /**
   * Build one month.
   */
  function calendar_build_month() {
    $translated_days = date_week_days_ordered(date_week_days(TRUE));
    $month = date_format($this->curday, 'n');
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
    $weekdays = calendar_untranslated_days($this->items, $this->view);
    date_modify($this->curday, '-' . strval(date_format($this->curday, 'j')-1) . ' days');
    $rows = array();
    do {
      $init_day = clone($this->curday);
      $today = date_format(date_now(date_default_timezone()), DATE_FORMAT_DATE);
      $month = date_format($this->curday, 'n');
      $week = date_week($curday_date);
      // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $first_day = variable_get('date_first_day', 0);

      $week_rows = $this->calendar_build_week(TRUE);
      $multiday_buckets = $week_rows['multiday_buckets'];
      $singleday_buckets = $week_rows['singleday_buckets'];
      $total_rows = $week_rows['total_rows'];

      // Theme each row
      $output = "";
      $final_day = clone($this->curday);

      $iehint = 0;
      $max_multirow_cnt = 0;
      $max_singlerow_cnt = 0;

      for ($i = 0; $i < intval($total_rows + 1); $i++) {
        $inner = "";

        // If we're displaying the week number, add it as the
        // first cell in the week.
        if ($i == 0 && !empty($this->date_info->style_with_weekno) && !in_array($this->date_info->granularity, array('day', 'week'))) {
          $path = calendar_granularity_path($this->view, 'week');
          if (!empty($path)) {
            $url = $path . '/' . $this->date_info->year . '-W' . $week;
            // @FIXME
// l() expects a Url object, created from a route name or external URI.
// $weekno = l($week, $url, array('query' => !empty($this->date_info->append) ? $this->date_info->append : ''));

          }
          else {
            // Do not link week numbers, if Week views are disabled.
            $weekno = $week;
          }
          $item = array(
            'entry' => $weekno,
            'colspan' => 1,
            'rowspan' => $total_rows + 1,
            'id' => $this->view->name . '-weekno-' . $curday_date,
            'class' => 'week',
          );
          // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $inner .= theme('calendar_month_col', array('item' => $item));

        }

        $this->curday = clone($init_day);

        // move backwards to the first day of the week
        $day_wday = date_format($this->curday, 'w');
        date_modify($this->curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');

        for ( $wday = 0; $wday < 7; $wday++) {

          $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
          $class = strtolower($weekdays[$wday]);
          $item = NULL;
          $in_month = !($curday_date < $this->date_info->min_date_date || $curday_date > $this->date_info->max_date_date || date_format($this->curday, 'n') != $month);

          // Add the datebox
          if ($i == 0) {
            $variables = array(
              'date' => $curday_date,
              'view' => $this->view,
              'items' => $this->items,
              'selected' => $in_month ? count($multiday_buckets[$wday]) + count($singleday_buckets[$wday]) : FALSE,
            );
            // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $item = array(
//               'entry' => theme('calendar_datebox', $variables),
//               'colspan' => 1,
//               'rowspan' => 1,
//               'class' => 'date-box',
//               'date' => $curday_date,
//               'id' => $this->view->name . '-' . $curday_date . '-date-box',
//               'header_id' => $translated_days[$wday],
//               'day_of_month' => $this->curday->format('j'),
//             );

            $item['class'] .= ($curday_date == $today && $in_month ? ' today' : '') .
              ($curday_date < $today ? ' past' : '') .
              ($curday_date > $today ? ' future' : '');
          }
          else {
            $index = $i - 1;
            $multi_count = count($multiday_buckets[$wday]);

            // Process multiday buckets first.  If there is a multiday-bucket item in this row...
            if ($index < $multi_count) {
              // If this item is filled with either a blank or an entry...
              if ($multiday_buckets[$wday][$index]['filled']) {

                // Add item and add class
                $item = $multiday_buckets[$wday][$index];
                $item['class'] =  'multi-day';
                $item['date'] = $curday_date;

                // Is this an entry?
                if (!$multiday_buckets[$wday][$index]['avail']) {

                  // If the item either starts or ends on today,
                  // then add tags so we can style the borders
                  if ($curday_date == $today && $in_month) {
                    $item['class'] .=  ' starts-today';
                  }

                  // Calculate on which day of this week this item ends on..
                  $end_day = clone($this->curday);
                  $span = $item['colspan'] - 1;
                  date_modify($end_day, '+' . $span . ' day');
                  $endday_date = date_format($end_day, DATE_FORMAT_DATE);

                  // If it ends today, add class
                  if ($endday_date == $today && $in_month) {
                    $item['class'] .=  ' ends-today';
                  }
                }
              }

              // If this is an actual entry, add classes regarding the state of the
              // item
              if ($multiday_buckets[$wday][$index]['avail']) {
                $item['class'] .= ' ' . $wday . ' ' . $index . ' no-entry ' . ($curday_date == $today && $in_month ? ' today' : '') .
                  ($curday_date < $today ? ' past' : '') .
                  ($curday_date > $today ? ' future' : '');
              }

            // Else, process the single day bucket - we only do this once per day
            }
            elseif ($index == $multi_count) {
              $single_day_cnt = 0;
              // If it's empty, add class
              if (count($singleday_buckets[$wday]) == 0) {
                $single_days = "&nbsp;";
                if ($max_multirow_cnt == 0 ) {
                  $class = ($multi_count > 0 ) ? 'single-day no-entry noentry-multi-day' : 'single-day no-entry';
                }
                else {
                  $class = 'single-day';
                }
              }
              else {
                $single_days = "";
                foreach ($singleday_buckets[$wday] as $day) {
                  foreach ($day as $event) {
                    $single_day_cnt++;
                    $single_days .= (isset($event['more_link'])) ? '<div class="calendar-more">' . $event['entry'] . '</div>' : $event['entry'];
                  }
                }
                $class = 'single-day';
              }

              $rowspan = $total_rows - $index;
              // Add item...
              $item = array(
                'entry' => $single_days,
                'colspan' => 1,
                'rowspan' => $rowspan,
                'class' => $class,
                'date' => $curday_date,
                'id' => $this->view->name . '-' . $curday_date . '-' . $index,
                'header_id' => $translated_days[$wday],
                'day_of_month' => $this->curday->format('j'),
              );

              // Hack for ie to help it properly space single day rows
              if ($rowspan > 1 && $in_month && $single_day_cnt > 0) {
                $max_multirow_cnt = max($max_multirow_cnt, $single_day_cnt);
              }
              else {
                $max_singlerow_cnt = max($max_singlerow_cnt, $single_day_cnt);
              }

              // If the singlerow is bigger than the multi-row, then null out
              // ieheight - I'm estimating that a single row is twice the size of
              // multi-row.  This is really the best that can be done with ie
              if ($max_singlerow_cnt >= $max_multirow_cnt || $max_multirow_cnt <= $multi_count / 2 ) {
                $iehint = 0;
              }
              elseif ($rowspan > 1 && $in_month && $single_day_cnt > 0) {
                $iehint = max($iehint, $rowspan - 1); // How many rows to adjust for?
              }

              // Set the class
              $item['class'] .= ($curday_date == $today && $in_month ? ' today' : '') .
                ($curday_date < $today ? ' past' : '') .
                ($curday_date > $today ? ' future' : '');
            }
          }

          // If there isn't an item, then add empty class
          if ($item != NULL) {
            if (!$in_month) {
              $item['class'] .= ' empty';
            }
            // Style this entry - it will be a <td>.
            // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
 $inner .= theme('calendar_month_col', array('item' => $item));

          }

          date_modify($this->curday, '+1 day');
        }

        if ($i == 0) {
          // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $output .= theme('calendar_month_row', array(
//             'inner' => $inner,
//             'class' => 'date-box',
//             'iehint' => $iehint,
//           ));

        }
        elseif ($i == $total_rows) {
          // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $output .= theme('calendar_month_row', array(
//             'inner' => $inner,
//             'class' => 'single-day',
//             'iehint' => $iehint,
//           ));

          $iehint = 0;
          $max_singlerow_cnt = 0;
          $max_multirow_cnt = 0;
        }
        else {
          // Style all the columns into a row
          // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $output .= theme('calendar_month_row', array(
//             'inner' => $inner,
//             'class' => 'multi-day',
//             'iehint' => 0,
//           ));

        }

      } // End foreach

      $this->curday = $final_day;

      // Add the row into the row array....
      $rows[] = array('data' => $output);

      $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
      $curday_month = date_format($this->curday, 'n');
    } while ($curday_month == $month && $curday_date <= $this->date_info->max_date_date);
    // Merge the day names in as the first row.
    $rows = array_merge(array(calendar_week_header($this->view)), $rows);
    return $rows;
  }

  /**
   * Build one week row.
   */
  function calendar_build_week($check_month = FALSE) {
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
    $weekdays = calendar_untranslated_days($this->items, $this->view);
    $month = date_format($this->curday, 'n');
    // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $first_day = variable_get('date_first_day', 0);


    // Set up buckets
    $total_rows = 0;
    $multiday_buckets = array( array(), array(), array(), array(), array(), array(), array());
    $singleday_buckets = array( array(), array(), array(), array(), array(), array(), array());

    // move backwards to the first day of the week
    $day_wday = date_format($this->curday, 'w');
    date_modify($this->curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);

    for ($i = 0; $i < 7; $i++) {
      if ($check_month && ($curday_date < $this->date_info->min_date_date || $curday_date > $this->date_info->max_date_date || date_format($this->curday, 'n') != $month)) {
        $class = strtolower($weekdays[$i]) . ' empty';
        // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $singleday_buckets[$i][][] = array(
//           'entry' => theme('calendar_empty_day', array(
//              'curday' => $curday_date,
//              'view' => $this->view,
//           )),
//           'item' => NULL
//         );

      }
      else {
        $this->calendar_build_week_day($i, $multiday_buckets, $singleday_buckets);
      }
      $total_rows = max(count($multiday_buckets[$i]) + 1, $total_rows);
      date_modify($this->curday, '+1 day');
      $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
    }

    $rows = array(
      'multiday_buckets' => $multiday_buckets,
      'singleday_buckets' => $singleday_buckets,
      'total_rows' => $total_rows);
    return $rows;
  }

  /**
   * Build the contents of a single day for the $rows results.
   */
  function calendar_build_week_day($wday, &$multiday_buckets, &$singleday_buckets) {
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
    $max_events = $this->date_info->calendar_type == 'month' && !empty($this->date_info->style_max_items) ? $this->date_info->style_max_items : 0;
    $hide = !empty($this->date_info->style_max_items_behavior) ? ($this->date_info->style_max_items_behavior == 'hide') : FALSE;
    $multiday_theme = !empty($this->date_info->style_multiday_theme) && $this->date_info->style_multiday_theme == '1';
    // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $first_day = variable_get('date_first_day', 0);

    $cur_cnt = 0;
    $total_cnt = 0;
    $ids = array();

    // If we are hiding, count before processing further
    if ($max_events != CALENDAR_SHOW_ALL) {
      foreach ($this->items as $date => $day) {
        if ($date == $curday_date) {
          foreach ($day as $time => $hour) {
            foreach ($hour as $key => $item) {
              $total_cnt++;
              $ids[] = $item->date_id;
            }
          }
        }
      }
    }

    // If we haven't already exceeded the max or we'll showing all, then process the items
    if ($max_events == CALENDAR_SHOW_ALL || !$hide || $total_cnt < $max_events) {
      // Count currently filled items
      foreach ($multiday_buckets[$wday] as $bucket) {
        if (!$bucket['avail']) {
          $cur_cnt++;
        }
      }
      foreach ($this->items as $date => $day) {
        if ($date == $curday_date) {
          ksort($day);
          foreach ($day as $time => $hour) {
            foreach ($hour as $key => $item) {
              $all_day = $item->calendar_all_day;

              // Parse out date part
              $start_ydate = date_format($item->date_start, DATE_FORMAT_DATE);
              $end_ydate = date_format($item->date_end, DATE_FORMAT_DATE);
              $cur_ydate = date_format($this->curday, DATE_FORMAT_DATE);

              $is_multi_day = ($start_ydate < $cur_ydate || $end_ydate > $cur_ydate);

              // Does this event span multi-days?
              if ($multiday_theme && ($is_multi_day || $all_day)) {

                // Remove multiday items from the total count. We can't hide them or they will break.
                $total_cnt--;

                // If this the first day of the week, or is the start date of the multi-day event,
                // then record this item, otherwise skip over
                $day_no = date_format($this->curday, 'd');
                if ($wday == 0 || $start_ydate == $cur_ydate || ($this->date_info->granularity == 'month' && $day_no == 1) || ($all_day && !$is_multi_day)) {
                  // Calculate the colspan for this event

                  // If the last day of this event exceeds the end of the current month or week,
                  // truncate the remaining days
                  $diff =  $this->curday->difference($this->date_info->max_date, 'days');
                  $remaining_days = ($this->date_info->granularity == 'month') ? min(6 - $wday, $diff) : $diff - 1;
                  // The bucket_cnt defines the colspan.  colspan = bucket_cnt + 1
                  $days = $this->curday->difference($item->date_end, 'days');
                  $bucket_cnt = max(0, min($days, $remaining_days));

                  // See if there is an available slot to add an event.  This will allow
                  // an event to precede a row filled up by a previous day event
                  $avail = FALSE;
                  $bucket_index = count($multiday_buckets[$wday]);
                  for ($i = 0; $i < $bucket_index; $i++) {
                    if ($multiday_buckets[$wday][$i]['avail']) {
                      $bucket_index = $i;
                      break;
                    }
                  }

                  // Add continuation attributes
                  $item->continuation =  ($item->date_start < $this->curday);
                  $item->continues = ( $days > $bucket_cnt );
                  $item->is_multi_day = TRUE;

                  // Assign the item to the available bucket
                  // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $multiday_buckets[$wday][$bucket_index] = array(
//                     'colspan' => $bucket_cnt + 1,
//                     'rowspan' => 1,
//                     'filled' => TRUE,
//                     'avail' => FALSE,
//                     'all_day' => $all_day,
//                     'item' => $item,
//                     'wday' => $wday,
//                     'entry' => theme('calendar_item', array('view' => $this->view, 'rendered_fields' => $item->rendered_fields, 'item' => $item)),
//                   );


                  // Block out empty buckets for the next days in this event for this week
                  for ($i = 0; $i < $bucket_cnt; $i++) {
                    $bucket = &$multiday_buckets[$i + $wday + 1];
                    $bucket_row_count = count($bucket);
                    $row_diff = $bucket_index - $bucket_row_count;

                    // Fill up the preceding buckets - these are available for future
                    // events
                    for ( $j = 0; $j < $row_diff; $j++) {
                      $bucket[($bucket_row_count + $j) ] = array(
                        'entry' => '&nbsp;',
                        'colspan' => 1,
                        'rowspan' => 1,
                        'filled' => TRUE,
                        'avail' => TRUE,
                        'wday' => $wday,
                        'item' => NULL
                      );
                    }
                    $bucket[$bucket_index] = array(
                      'filled' => FALSE,
                      'avail' => FALSE
                    );
                  }
                }
              }
              elseif ($max_events == CALENDAR_SHOW_ALL || $cur_cnt < $max_events) {
                $cur_cnt++;
                // Assign to single day bucket
                // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $singleday_buckets[$wday][$time][] = array(
//                   'entry' => theme('calendar_item', array('view' => $this->view, 'rendered_fields' => $item->rendered_fields, 'item' => $item)),
//                   'item' => $item,
//                   'colspan' => 1,
//                   'rowspan' => 1,
//                   'filled' => TRUE,
//                   'avail' => FALSE,
//                   'wday' => $wday,
//                 );

              }

            }
          }
        }
      }
    }

    // Add a more link if necessary
    if ($max_events != CALENDAR_SHOW_ALL && $total_cnt > 0 && $cur_cnt < $total_cnt) {
      // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $entry = theme('calendar_' . $this->date_info->calendar_type . '_multiple_entity', array(
//           'curday' => $curday_date,
//           'count' => $total_cnt,
//           'view' => $this->view,
//           'ids' => $ids,
//         ));

      if (!empty($entry)) {
        $singleday_buckets[$wday][][] = array(
          'entry' => $entry,
          'more_link' => TRUE,
          'item' => NULL
        );
      }
    }
  }

  /**
   * Build the contents of a single day for the $rows results.
   */
  function calendar_build_day() {
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
    $selected = FALSE;
    $max_events = !empty($this->date_info->style_max_items) ? $this->date_info->style_max_items : 0;
    $ids = array();
    $inner = array();
    $all_day = array();
    $empty = '';
    $link = '';
    $count = 0;
    foreach ($this->items as $date => $day) {
      if ($date == $curday_date) {
        $count = 0;
        $selected = TRUE;
        ksort($day);
        foreach ($day as $time => $hour) {
          foreach ($hour as $key => $item) {
            $count++;
            if (isset($item->type)) {
              $ids[$item->type] = $item;
            }
            if (empty($this->date_info->mini) && ($max_events == CALENDAR_SHOW_ALL || $count <= $max_events || ($count > 0 && $max_events == CALENDAR_HIDE_ALL))) {
              if ($item->calendar_all_day) {
                $item->is_multi_day = TRUE;
                $all_day[] = $item;
              }
              else {
                $key = date_format($item->calendar_start_date, 'H:i:s');
                $inner[$key][] = $item;
              }
            }
          }
        }
      }
    }
    ksort($inner);

    if (empty($inner) && empty($all_day)) {
      // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $empty = theme('calendar_empty_day', array('curday' => $curday_date, 'view' => $this->view));

    }
    // We have hidden events on this day, use the theme('calendar_multiple_') to show a link.
    if ($max_events != CALENDAR_SHOW_ALL && $count > 0 && $count > $max_events && $this->date_info->calendar_type != 'day' && !$this->date_info->mini) {
      if ($this->date_info->style_max_items_behavior == 'hide' || $max_events == CALENDAR_HIDE_ALL) {
        $all_day = array();
        $inner = array();
      }
      // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $link = theme('calendar_' . $this->date_info->calendar_type . '_multiple_node', array(
//         'curday' => $curday_date,
//         'count' => $count,
//         'view' => $this->view,
//         'ids' => $ids,
//       ));

    }

    // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $content = array(
//       'date' => $curday_date,
//       'datebox' => theme('calendar_datebox', array(
//          'date' => $curday_date,
//          'view' => $this->view,
//          'items' => $this->items,
//          'selected' => $selected,
//       )),
//       'empty' => $empty,
//       'link' => $link,
//       'all_day' => $all_day,
//       'items' => $inner,
//       );

    return $content;
  }

  /**
   * Build one mini month.
   */
  function calendar_build_mini_month() {
    $month = date_format($this->curday, 'n');
    date_modify($this->curday, '-' . strval(date_format($this->curday, 'j')-1) . ' days');
    $rows = array();
    do {
      $rows = array_merge($rows, $this->calendar_build_mini_week());
      $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
      $curday_month = date_format($this->curday, 'n');
    } while ($curday_month == $month && $curday_date <= $this->date_info->max_date_date);
    // Merge the day names in as the first row.
    $rows = array_merge(array(calendar_week_header($this->view)), $rows);
    return $rows;
  }

  /**
   * Build one week row.
   */
  function calendar_build_mini_week($check_month = TRUE) {
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
    $weekdays = calendar_untranslated_days($this->items, $this->view);
    $today = date_format(date_now(date_default_timezone()), DATE_FORMAT_DATE);
    $month = date_format($this->curday, 'n');
    $week = date_week($curday_date);
    // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $first_day = variable_get('date_first_day', 0);

    // move backwards to the first day of the week
    $day_wday = date_format($this->curday, 'w');
    date_modify($this->curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');
    $curday_date = date_format($this->curday, DATE_FORMAT_DATE);

    if (!empty($this->date_info->style_with_weekno)) {
      $path = calendar_granularity_path($this->view, 'week');
      if (!empty($path)) {
        $url = $path . '/' . $this->date_info->year . '-W' . $week;
        // @FIXME
// l() expects a Url object, created from a route name or external URI.
// $weekno = l($week, $url, array('query' => !empty($this->date_info->append) ? $this->date_info->append : ''));

      }
      else {
        // Do not link week numbers, if Week views are disabled.
        $weekno = $week;
      }
      $rows[$week][] = array(
        'data' => $weekno,
        'class' => 'mini week',
        'id' => $this->view->name . '-weekno-' . $curday_date,
      );
    }

    for ($i = 0; $i < 7; $i++) {
      $curday_date = date_format($this->curday, DATE_FORMAT_DATE);
      $class = strtolower($weekdays[$i] . ' mini');
      if ($check_month && ($curday_date < $this->date_info->min_date_date || $curday_date > $this->date_info->max_date_date || date_format($this->curday, 'n') != $month)) {
         $class .= ' empty';
         $variables = array(
          'curday' => $curday_date,
          'view' => $this->view,
        );

        // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $content = array(
//           'date' => '',
//           'datebox' => '',
//           'empty' => theme('calendar_empty_day', $variables),
//           'link' => '',
//           'all_day' => array(),
//           'items' => array(),
//           );

      }
      else {
        $content = $this->calendar_build_day();
        $class .= ($curday_date == $today ? ' today' : '') .
          ($curday_date < $today ? ' past' : '') .
          ($curday_date > $today ? ' future' : '') .
          (empty($this->items[$curday_date]) ? ' has-no-events' : ' has-events');
      }
      $rows[$week][] = array(
        'data' => $content,
        'class' => $class,
        'id' => $this->view->name . '-' . $curday_date,
      );
      date_modify($this->curday, '+1 day');
    }
    return $rows;
  }

}
