<?php
// $Id$

/**
 * Display a month view.
 */
function template_preprocess_calendar_month(&$vars) {
  
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
  
  $rows = $vars['rows'];
  if (sizeof($rows) > 1) {
    $day_names = array_shift($rows);
  }
  else {
    $day_names = $rows;
    $rows = array();
  }
  $vars['rows'] = $rows;
  $vars['day_names'] = $day_names;
}

/**
 * Display a year view.
 */
function template_preprocess_calendar_year(&$vars) {
  $original_vars = $vars;
  
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
  
  $view = $vars['view'];
  $rows = $vars['rows'];
  
  $months = array();
  
  foreach ($rows as $month => $month_rows) {
    $view->month = $month;
    $view->min_date = date_make_date($view->year .'-'. date_pad($month) .'-01 00:00:00', date_default_timezone_name());
    $view->max_date = drupal_clone($view->min_date);
    date_modify($view->max_date, '+1 month');
    date_modify($view->max_date, '-1 second');
    $month_header = array(array('data' => theme('calendar_nav_title', 'month', $view), 'colspan' => 7));
    $months[$month] = theme('calendar_month', $view, $vars['options'], $month_rows);
  }
  $vars = $original_vars;
  $var['months'] = $months;
}

/**
 * Display a day view.
 */
function template_preprocess_calendar_day(&$vars) {
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
}

/**
 * Display a week view.
 */
function template_preprocess_calendar_week(&$vars) {
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
}

/**
 *  Preprocessor to construct back and next navigation from views arguments
 */
function template_preprocess_calendar_nav(&$vars) {
  
  $view = $vars['view'];
  $with_weekno = $vars['with_weekno'];
  $min_date = is_object($view->min_date) ? $view->min_date : date_now();
  $max_date = is_object($view->max_date) ? $view->max_date : date_now();
  
  if (!is_object($view->min_date)) {
    return;
  }
  $prev_date = drupal_clone($min_date);
  date_modify($prev_date, '-1 '. $view->calendar_type);
  $next_date = drupal_clone($min_date);
  date_modify($next_date, '+1 '. $view->calendar_type);

  $format = array('year' => 'Y', 'month' => 'Y-m', 'day' => 'Y-m-d', 'week' => 'Y\Ww');
 
  switch ($view->calendar_type) {
    case 'week':
      break;
    default:
      $next_path = str_replace($view->calendar_arg, date_format($next_date, $format[$view->calendar_type]), $view->get_url());
      $prev_path = str_replace($view->calendar_arg, date_format($prev_date, $format[$view->calendar_type]), $view->get_url());
  }
  $vars['next_url'] = $next_path;
  $vars['prev_url'] = $prev_path;
  
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
  $vars['colspan_prev'] = $colspan_prev;
  $vars['colspan_next'] = $colspan_next;
  $vars['colspan_middle'] = $colspan_middle;
  
  if ($view->mini) {
	  // Month navigation titles are used as links in the mini view.
	  $nav_title = l(date_format_date($view->min_date, 'custom', 'M Y'), $view->url .'/'. $view->year .'/'. $view->month, array(), calendar_url_append($view));
  }
  else {
    // Otherwise, just show the date.
    switch ($view->calendar_type) {
      case 'day':
        $nav_title = date_format_date($view->min_date, 'custom', 'l, F j Y');
      case 'week':
        $nav_title = t('Week of @date', array('@date' => date_format($view->min_date, 'F j')));
      default:
        $nav_title = date_format_date($view->min_date, 'custom', 'M Y');
    }
  }
  $vars['nav_title'] = $nav_title;
  
  $header = array();
  $header[]  = array('data' => theme('calendar_nav_prev', $prev_path, $view->build_type == 'block' ? FALSE : TRUE), 'class' => 'prev', 'colspan' => $colspan_prev);
  $header[]  = array('data' => $view->subtitle, 'class' => 'heading', 'colspan' => $colspan_middle);
  $header[]  = array('data' => theme('calendar_nav_next', $next_path, $view->build_type == 'block' ? FALSE : TRUE), 'class' => 'next', 'colspan' => $colspan_next);

  $vars['calendar_nav'] = $header;
}

/**
 * Create the calendar date box.
 */
function template_preprocess_calendar_datebox(&$vars) {
  $date = $vars['date'];
  $view = $vars['view'];
  
  $vars['day'] = intval(substr($date, 8, 2));
  $vars['url'] = str_replace($view->calendar_arg, substr($date, 0, 10), $view->get_url());
  $vars['link'] = l($vars['day'], $vars['url']);
  $vars['calendar_type'] = $view->calendar_type;
  $vars['mini'] = !empty($view->mini);
  
  if ($view->mini) {
    if ($selected) {
      $vars['class'] = 'mini-day-on';
    }
    else {
      $vars['class'] = 'mini-day-off';
    }
  }
  else {
    $vars['class'] = 'day';
  }
}

/**
 * @addtogroup themeable
 * @{
 */
/**
 * Display the nodes of a view as a list.
 */
function theme_calendar_view_list($view, $nodes, $type) {
  $output  = '<div class="calendar-calendar">';
  $output .= theme('calendar_links', $view, $view->build_type != 'block');
  $output .= theme("table", calendar_nav($view), array());
  $output .= '</div>';

  $fields = views_fetch_fields('node', 'field');
  $items = array();
  foreach ($nodes as $node) {
    $item = '';
    foreach ($view->field as $field) {
      if ($fields[$field['id']]['visible'] !== FALSE) {
        $value = $node->$field['field'];
        if (!empty($value)) {
          if ($field['label']) {
            $item .= "<div class='view-label ". views_css_safe('view-label-'. $field['queryname']) ."'>". $field['label'] ."</div>";
          }
          $item .= "<div class='view-field ". views_css_safe('view-data-'. $field['queryname']) ."'>";
          $item .= $value ."</div>";
        }
      }
    }
    $items[] = "<div class='view-item ". views_css_safe('view-item-'. $view->name) ."'>$item</div>\n"; // l($node->title, "node/$node->nid");
  }
  if (!empty($items)) {
   	$output .= theme('item_list', $items);
  }
  else {
    $output .= views_get_textarea($view, $type, 'empty');
  }
  return $output;
}

/**
 * Display the nodes of a view as a table.
 */
function theme_calendar_view_table($view, $nodes, $type) {
  $output  = '<div class="calendar-calendar">';
  $output .= theme('calendar_links', $view, $view->build_type != 'block');
  $output .= theme("table", calendar_nav($view), array());
  $output .= '</div>';

  $fields = views_fetch_fields('node', 'field');
  $rows = array();
  foreach ($nodes as $node) {
    $row = array();
    foreach ($view->field as $field) {
      if ($fields[$field['id']]['visible'] !== FALSE) {
        $cell['data'] = $value = $node->$field['field'];
        $cell['class'] = "view-field ". views_css_safe('view-field-'. $field['queryname']);
        $row[] = $cell;
      }
    }
    $rows[] = $row;
  }
  if (!empty($rows)) {
    $output .= theme('table', $view->table_header, $rows);
  }
  else {
    $output .= views_get_textarea($view, $type, 'empty');
  }
  return $output;
}

/**
 * Display the nodes of a view as teasers.
 */
function theme_calendar_view_teasers($view, $nodes, $type) {
  return views_theme('calendar_view_nodes', $view, $nodes, 'teasers', TRUE);
}

/**
 * Display the nodes of a view as plain nodes.
 */
function theme_calendar_view_nodes($view, $nodes, $type, $teasers = false, $links = true) {
  $output  = '<div class="calendar-calendar">';
  $output .= theme('calendar_links', $view, $view->build_type != 'block');
  $output .= theme("table", calendar_nav($view), array());
  $output .= '</div>';

  $data = array();
  foreach ($nodes as $n) {
    if (isset($n->calendar_node_theme)) {
      $theme = $n->calendar_node_theme;
      $data[] = theme($theme, $n, $view);
    }
    else {
      $node = node_load($n->nid);
      $data[] = node_view($node, $teasers, false, $links);
    }
  }
  if (!empty($data)) {
    $output .= implode($data);
  }
  else {
    $output .= views_get_textarea($view, $type, 'empty');
  }
  return $output;
}

/**
 * Theme the calendar page title.
 *
 * Views defaults to displaying the title of the last argument in the
 * view, rather than the View title or other values. Use this theme
 * to override that behavior.
 *
 * $view->build_type indicates whether this view is being rendered as a page
 * or a block, use that to handle the title differently for each.
 *
 * views_get_title() $context can be:
 *  'page' - The title that goes with the last argument in $args.
 *  'menu' - The value in View Menu Title.
 *
 * or just use the values of:
 *  $view->page_title,
 *  $view->menu_title,
 *  $view->block_title.
 */
function theme_calendar_page_title($view, $items, $output) {
  switch ($view->build_type) {
    case 'page':
      if ($view->calendar_type == 'year') {
        return $view->year;
      }
      else {
        return views_get_title($view, $context = 'page', $args = $view->args);
      }
    case 'block':
      return $view->block_title;
  }
}

/**
 * Theme the calendar title and breadcrumbs
 *   Arguments are evaluated in year, month, day or year, week order
 *   so you can track previous values in the session.
 *
 * @param string $field_type - 'YEAR', 'MONTH', 'DAY', 'WEEK'
 * @param integer $value - the current number for the field type as selected in the view argument.
 * @return string formatted title
 */
function theme_calendar_arg_title($field_type, $value, $query) {
  $view = $GLOBALS['current_view'];
  $value = intval(check_plain($value));
  if (empty($value)) {
    if ($view->month) {
      return date_format_date($view->min_date, 'custom', 'F Y');
    }
    elseif ($view->year) {
     return $view->year;
    }
	}
	else {
	  switch ($field_type) {
    case 'YEAR':
      $view->year = $value;
      return $view->year;
    case 'MONTH':
      return date_format_date($view->min_date, 'custom', 'F');
    case 'DAY':
      return date_format_date($view->min_date, 'custom', 'l, F j Y');
    case 'WEEK':
    	return t('Week of @date', array('@date' => date_format($view->min_date, 'F j')));
   }
	}
}

/**
 * Format a node stripe legend
 */
function theme_calendar_stripe_legend($stripe_labels) {
  $header = array(
      array('class' => 'legend', 'data' => t('Item')),
      array('class' => 'legend', 'data' => t('Key'))
      );
  foreach ($stripe_labels as $stripe => $label) {
    $node = new StdClass();
    $node->stripe = $stripe;
    $rows[] = array($label, theme('calendar_stripe_stripe', $node), array('class' => 'stripe'));
  }
  $output = theme('table', $header, $rows, array('class' => 'mini'));
  return $output;
}

/**
 * Format node stripes
 * Add key value to text, then hide it with css for accessibility to screen readers
 */
function theme_calendar_stripe_stripe($node) {
  static $stripe, $stripe_map;

  if(!$stripe_map[$node->stripe]) {
    if($stripe >= 10) {
      $stripe = 1;
    }
    else {
      $stripe++;
    }
    $stripe_map[$node->stripe] = $stripe;
  }

  $output .= '<div class="stripe-'. $stripe_map[$node->stripe] .'" title="Key: '.$stripe_map[$node->stripe] .'"><span class="stripe">Key '.$stripe_map[$node->stripe] .'</span></div>'."\n";
  return $output;
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */
function theme_calendar_node_day($node, $view) {
  $output = '<div class="calendar dayview">'."\n";
  $output .= theme('calendar_stripe_stripe', $node);
  $fields = views_fetch_fields('node', 'field');
  $item = '';
  foreach ($view->field as $field) {
    if ($fields[$field['id']]['visible'] !== FALSE) {
      $$value = $node->$field['field'];
      if (!empty($value)) {
        if ($field['label']) {
          $item .= "<div class='view-label ". views_css_safe('view-label-'. $field['queryname']) ."'>". $field['label'] ."</div>";
        }
        $item .= "<div class='view-field ". views_css_safe('view-data-'. $field['queryname']) ."'>";
        $item .= $value ."</div>";
      }
    }
  }
  // Remote items may have a teaser to show.
  if ($node->remote && $node->teaser) {
    $item .= '<div class="content">'. ($node->teaser) ."</div>\n";
  }
  $output .= "<div class='view-item ". views_css_safe('view-item-'. $view->name) ."'>$item</div>\n";
  $output .= "</div>\n";
  return $output;
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */

function theme_calendar_node_week($node, $view) {
  $output .= '<div class="calendar weekview">'."\n";
  $output .= theme('calendar_stripe_stripe', $node);
  $fields = views_fetch_fields('node', 'field');
  $item = '';
  foreach ($view->field as $field) {
    if ($fields[$field['id']]['visible'] !== FALSE) {
      // Skip the label in this small box, show only the value;
      $value = $node->$field['field'];
      if (!empty($value)) {
        $item .= "<div class='view-field ". views_css_safe('view-data-'. $field['queryname']) ."'>";
        $item .= $value ."</div>";
      }
    }
  }
  $output .= "<div class='view-item ". views_css_safe('view-item-'. $view->name) ."'>$item</div>\n";
  $output .= '</div>' . "\n";
  return $output;
}

/**
 * Format an calendar node for display in an expanded calendar, like a calendar page
 *
 * @param node
 *   The node being displayed
 */
function theme_calendar_node_month($node, $view) {
  $output .= '<div class="calendar monthview">'."\n";
  $output .= theme('calendar_stripe_stripe', $node);
  $fields = views_fetch_fields('node', 'field');
  $item = '';
  foreach ($view->field as $field) {
    if ($fields[$field['id']]['visible'] !== FALSE) {
      $value = $node->$field['field'];
      if (!empty($value)) {
        $item .= "<div class='view-field ". views_css_safe('view-data-'. $field['queryname']) ."'>";
        $item .= $value ."</div>";
      }
    }
  }
  $output .= "<div class='view-item ". views_css_safe('view-item-'. $view->name) ."'>$item</div>\n";
  $output .= "</div>\n";
  return $output;
}

/**
 * Format an empty day on a calendar
 *
 * @param day
 *   The day to display.
 */
function theme_calendar_empty_day() {
  return '<div class="calendar-empty">&nbsp;</div>'."\n";
}

/**
 * Format a from/to date in the calendar view.
 *
 * Alter the display as appropriate for the type of view.
 * We have to do our own display of the date because we altered the
 * value to the local timezone, and the built-in formatters assume
 * they're operating on a UTC date. Plus we can fine-tune the display
 * to show only the time in the calendar month and week cells but the
 * whole date in other places.
 */
function theme_calendar_date_combo($node, $label, $view) {
  switch ($view->calendar_display) {
    // Some of the calendar views need a smaller date format.
    case 'calendar':
      switch ($view->calendar_type) {
        case 'year':
          // We don't display individual dates in the calendar year view.
          return;
        case 'week':
        case 'month':
          // No room for a label or the full date in these small
          // displays, show only the time.
          $format = $node->format_time;
          $label = '';
          break;
        case 'day':
          $format = $node->format;
          break;
      }
      break;
    // For non-calendar views, like lists and tables, show the entire date.
    default:
      $format = $node->format;
      break;
  }
  $date1 = date_format_date($node->calendar_start_date, 'custom', $format);
  $date2 = date_format_date($node->calendar_end_date, 'custom', $format);

  // No date values, display nothing.
  if (empty($date1) && empty($date2)) {
    $output = '';
  }
  // From and To dates match or there is no To date,
  // display a complete single date.
  elseif ($date1 == $date2 || empty($date2)) {
    $output = '<span class="date-display-single">'. $date1 .'</span>';
  }
  // Full date format, same day, different times, don't repeat the date
  // but show both From and To times.
  elseif (date_format($node->calendar_start_date, $node->format_time) != date_format($node->calendar_end_date, $node->format_time) && $format != $node->format_time) {
    $date_format = date_limit_format($format, array('year', 'month', 'day'));
    $output = '<span class="date-display-single">'. date_format($node->calendar_start_date, $date_format).'</span> '.
      '<span class="date-display-start">'. date_format($node->calendar_start_date, $node->format_time) .'</span>'.
      '<span class="date-display-separator"> - </span>'.
      '<span class="date-display-end">'. date_format($node->calendar_end_date, $node->format_time) .'</span>';
  }
  // Time format only or different days, display both in their entirety.
  else {
    $output = '<span class="date-display-start">'. $date1 .'</span>'.
      '<span class="date-display-separator"> - </span>'.
      '<span class="date-display-end">'. $date2 .'</span>';
  }
  return $output;
}


/** @} End of addtogroup themeable */