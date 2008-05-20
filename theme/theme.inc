<?php
// $Id$
/**
 * Display a view as a calendar.
 * 
 * This preprocessor does all the work needed for all types of calendar
 * views and the template takes care of displaying links to related views.
 */
function template_preprocess_calendar(&$vars) {
  include_once('./'. drupal_get_path('module', 'calendar') .'/calendar.inc');
  
  $view     = $vars['view'];
  // Make sure we only run through this function one time.
  if ($view->calendar_processed) {
    return;
  }
    
  $result   = (array) $view->result;
  $options  = $view->style_plugin->options;
  $handler  = $view->style_plugin;

  $fields   = $view->field;
  
  // Render each field into an output array. We have to do the rendering
  // here because we don't apppear to have full access to the view 
  // handlers in the theme functions.
  $items = array();
  $calendar_fields = array_keys(calendar_fields('alias'));
  foreach ($result as $num => $row) {
    $items[$num] = $row;
    // Store the raw date values before formatting the results.
    foreach ($row as $key => $value) {
      if (in_array($key, $calendar_fields)) {
        $items[$num]->calendar_fields->$key = $value;
      }
    }
    foreach ($fields as $field) {
      if (!empty($field['handler']) && is_object($field['handler'])) {
        $handler = &$field['handler'];
        $field_output = $handler->theme($row);
        $items[$num]->{$handler->field_alias} = $field_output;
      }
    }
  }
  $vars['display_type'] = $view->calendar_type;
  $vars['min_date_formatted'] = date_format($view->min_date, DATE_FORMAT_DATETIME);
  $vars['max_date_formatted'] = date_format($view->max_date, DATE_FORMAT_DATETIME);
  
  // Massage the resulting items into formatted calendar items.
  $items = calendar_build_nodes($view, $items);
    
  // Merge in items from other sources.
  foreach (module_implements('calendar_add_items') as $module) {
    $function = $module .'_calendar_add_items';
    if (function_exists($function)) {
      if ($feeds = $function($view)) {
        foreach ($feeds as $feed) {
          $items = $feed;
        }
      }
    }
  }

  // Construct a formatted title for the view from the calendar type.
  $view->subtitle = theme('calendar_nav_title', $view->calendar_type, $view);

  $params = array('with_weekno' => $calendar_type == 'month');
  $view->mini = isset($view->mini) ? $view->mini : $view->calendar_type == 'year';
  
  $vars['calendar_nav'] = theme('calendar_nav', $view, $params['with_weekno']);
  
  // Create the calendar day names and rows.
  $rows = calendar_build_calendar($view, $items, $params);
  $vars['items'] = $items;
  $vars['rows'] = $rows;

  // Create the links to other displays.
  $now = date_now();
  $url = $view->get_url();
  $arg = $view->calendar_arg;
  $view->month    = $view->month && $view->month != $view->argument['calendar_date']['wildcard'] ? $view->month : date_format($now, 'm');
  $view->day      = $view->day && $view->day != $view->argument['calendar_date']['wildcard'] ? $view->day : date_format($now, 'j');
  if (empty($view->week) || $view->week == $view->argument['calendar_date']['wildcard']) {
    $view->week = calendar_date_week($view->year .'-'. date_pad($view->month) .'-'. date_pad($view->day));
  }
  $displays = $view->style_options;
  if (!empty($displays['year'])) {
    $calendar_links[] = array('title' => t('Year'), 'href' => str_replace($arg, $view->year, $url));
  }
  if (!empty($displays['month'])) {
    $calendar_links[] = array('title' => t('Month'), 'href' => str_replace($arg, $view->year .'-'. $view->month, $url));
  }
  if (!empty($displays['week'])) {
    $calendar_links[] = array('title' => t('Week'), 'href' => str_replace($arg, $view->year .'-W'. $view->week, $url));
  }
  if (!empty($displays['day'])) {
    $calendar_links[] = array('title' => t('Day'), 'href' => str_replace($arg, $view->year .'-'. $view->month .'-'. $view->day, $url));
  }
  $vars['calendar_links'] = $calendar_links;
    
  // If the Date Popup module is enabled, add a popup date selector.
  if (module_exists('date_popup')) {
    //$output = '<div class="clear-block">'. calendar_date_select($view) .'</div>';
  }
 
  $view->calendar_processed = TRUE;
  $vars['view'] = $view;  
}

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
  $vars['mini'] = $vars['options']['mini'];
  
  $view = $vars['view'];
  $vars['display_type'] = $view->calendar_type;
  $vars['min_date_formatted'] = date_format($view->min_date, DATE_FORMAT_DATETIME);
  $vars['max_date_formatted'] = date_format($view->max_date, DATE_FORMAT_DATETIME);
}

/**
 * Display a year view.
 */
function template_preprocess_calendar_year(&$vars) {
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
  
  // Construct a calendar for each month, adjusting the $view passed 
  // to the theme so it will produce the right results.  
  $view = drupal_clone($vars['view']);
  $rows = $vars['rows'];
  $months = array();
  foreach ($rows as $month => $month_rows) {
    $view->month = $month;
    $view->calendar_type = 'month';
    $view->mini = TRUE;
    $view->hide_nav = TRUE;
    $view->with_weekno = FALSE;
    $view->min_date = date_make_date($view->year .'-'. date_pad($month) .'-01 00:00:00', date_default_timezone_name());
    $view->max_date = drupal_clone($view->min_date);
    
    date_modify($view->max_date, '+1 month');
    date_modify($view->max_date, '-1 second');
    $months[$month] = theme('calendar_month', $view, $vars['options'], $month_rows);
  }
  $vars['months'] = $months;
}

/**
 * Display a day view.
 */
function template_preprocess_calendar_day(&$vars) {
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
 * Display a week view.
 */
function template_preprocess_calendar_week(&$vars) {
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
 *  Preprocessor to construct back and next navigation from views arguments
 */
function template_preprocess_calendar_nav(&$vars) {
  
  $view = $vars['view'];
  $with_weekno = $view->with_weekno;
  
  if (!isset($view->hide_nav)) {
    $min_date = is_object($view->min_date) ? $view->min_date : date_now();
    $max_date = is_object($view->max_date) ? $view->max_date : date_now();
    $prev_date = drupal_clone($min_date);
    date_modify($prev_date, '-1 '. $view->calendar_type);
    $next_date = drupal_clone($min_date);
    date_modify($next_date, '+1 '. $view->calendar_type);
    $format = array('year' => 'Y', 'month' => 'Y-m', 'day' => 'Y-m-d');
    switch ($view->calendar_type) {
      case 'week':
        $next_week = calendar_date_week(date_format($next_date, 'Y-m-d'));
        $prev_week = calendar_date_week(date_format($prev_date, 'Y-m-d'));
        $next_path = str_replace($view->calendar_arg, date_format($next_date, 'Y-\W') . $next_week, $view->get_url());
        $prev_path = str_replace($view->calendar_arg, date_format($prev_date, 'Y-\W') . $prev_week, $view->get_url());
        break;
      default:
        $next_path = str_replace($view->calendar_arg, date_format($next_date, $format[$view->calendar_type]), $view->get_url());
        $prev_path = str_replace($view->calendar_arg, date_format($prev_date, $format[$view->calendar_type]), $view->get_url());
    }
    switch ($view->calendar_type) {
      case 'day':
      case 'year':
        $colspan_prev = $colspan_next = $colspan_middle = 1;
        break;
      default:
        $colspan_prev = $colspan_next = 2;
        $colspan_middle = $with_weekno ? 4 : 3;
        break;
    }
  }
  else {
    $next_path = '';
    $prev_path = '';
    $colspan_prev = $colspan_next = 0;
    $colspan_middle = $with_weekno ? 8 : 7;   
  }
  $vars['next_url'] = $next_path;
  $vars['prev_url'] = $prev_path;
  $vars['colspan_prev'] = $colspan_prev;
  $vars['colspan_next'] = $colspan_next;
  $vars['colspan_middle'] = $colspan_middle;
  
  if ($view->mini && $view->calendar_type == 'month') {
	  // Month navigation titles are used as links in the mini view.
	  $nav_title = l(date_format_date($view->min_date, 'custom', 'M'), str_replace($view->calendar_arg, date_format($view->min_date, 'Y-m'), $view->get_url()));
  }
  else {
    // Otherwise, just show the date.
    switch ($view->calendar_type) {
      case 'day':
        $nav_title = date_format_date($view->min_date, 'custom', 'l, F j Y');
        break;
      case 'week':
        $nav_title = t('Week of @date', array('@date' => date_format($view->min_date, 'F j')));
        break;
      case 'year':
        $nav_title = date_format_date($view->min_date, 'custom', 'Y');
        break;
      default:
        $nav_title = date_format_date($view->min_date, 'custom', 'M Y');
    }
  }
  $vars['nav_title'] = $nav_title;
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
  $vars['mini'] = $view->mini;
  
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
 * Format an calendar node for display.
 */
function template_preprocess_calendar_node(&$vars) {
  $node = $vars['node'];
  $view = $vars['view'];
  $fields = array();
  foreach ($view->field as $field) {
    $fields[$field['handler']->field_alias] = array(
      'id' => views_css_safe($field_alias),
      'label' => $field['label'],
      'data' => $node->{$field['handler']->field_alias},
      );
  }
  $vars['fields'] = $fields;
  $vars['calendar_start'] = $node->calendar_start;
  $vars['calendar_end'] = $node->calendar_end;
  $vars['calendar_start_date'] = $node->calendar_start_date;
  $vars['calendar_end_date'] = $node->calendar_end_date;
  
  // We added the node type to the results in the query,
  // but it will show up as $node->node_type instead of
  // $node->type. Rename it to match the normal way it 
  // would show up on a node object.
  $vars['node']->type = $vars['node']->node_type;
}

/**
 * Format an calendar month node for display.
 */
function template_preprocess_calendar_month_node(&$vars) {
  template_preprocess_calendar_node($vars);
}

/**
 * Format an calendar day node for display.
 */
function template_preprocess_calendar_day_node(&$vars) {
  template_preprocess_calendar_node($vars);
  // Remote items may have a teaser to show.
  if ($node->remote && $node->teaser) {
    $fields['teaser'] = '<div class="content">'. ($node->teaser) ."</div>\n";
  }
}

/**
 * Format an calendar week node for display.
 */
function template_preprocess_calendar_week_node(&$vars) {
  template_preprocess_calendar_node($vars);
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