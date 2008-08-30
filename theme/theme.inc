<?php
// $Id$
/**
 * Display a calendar navigation and links
 */
function template_preprocess_calendar_main(&$vars) {
  require_once('./'. drupal_get_path('module', 'calendar') .'/calendar.inc');
  $view     = $vars['view'];

  $result   = (array) $view->result;
  $options  = $view->style_plugin->options;
  $handler  = $view->style_plugin;

  $vars['display_type'] = $view->granularity;
  $vars['min_date_formatted'] = date_format($view->min_date, DATE_FORMAT_DATETIME);
  $vars['max_date_formatted'] = date_format($view->max_date, DATE_FORMAT_DATETIME);
  

  $view->mini = isset($view->mini) ? $view->mini : $view->granularity == 'year';

  // Construct a formatted title for the view from the calendar type.
  $view->subtitle = theme('calendar_title', $view->granularity, $view);
  
  // Create the links to other displays.
  $now = date_now();
  $url = $view->get_url();
  $view->url = $url;
  $arg = $view->date_arg;
  $view->month    = $view->month && $view->month != $view->argument['date_argument']->options['wildcard'] ? $view->month : date_format($now, 'm');
  $view->day      = $view->day && $view->day != $view->argument['date_argument']->options['wildcard'] ? $view->day : date_format($now, 'd');
  if (empty($view->week) || $view->week == $view->argument['date_argument']->options['wildcard']) {
    $view->week = date_week($view->year .'-'. date_pad($view->month) .'-'. date_pad($view->day));
  }
  $displays = $view->display_types;
    
  $calendar_links = array();
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
  $vars['view'] = $view;  
  $vars['mini'] = !empty($view->mini);
  $vars['block'] = !empty($view->block);
  $vars['block_identifier'] = isset($view->block_identifier) ? $view->block_identifier : 'mini';
    
}

/**
 * Display a view as a calendar.
 * 
 * This preprocessor does all the work needed for all types of calendar
 * views and the template takes care of displaying links to related views.
 */
function template_preprocess_calendar(&$vars) {
  require_once('./'. drupal_get_path('module', 'calendar') .'/calendar.inc');
  $view     = $vars['view'];
  
  // Make sure we only run through this function one time.
  if (!empty($view->calendar_processed)) {
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
  $calendar_fields = date_api_fields();
  $calendar_fields = array_keys($calendar_fields['alias']);
  foreach ($result as $num => $row) {
    $items[$num] = $row;
    // Store the raw date values before formatting the results.
    foreach ($row as $key => $value) {
      if (in_array($key, $calendar_fields)) {
        $items[$num]->calendar_fields->$key = $value;
      }
    }
    foreach ($fields as $field) {
      if (!empty($field->options['exclude'])) {
        unset($items[$num]->{$field->field_alias});
      }
      elseif (!empty($field) && is_object($field)) {
        $field_output = $field->theme($row);
        $items[$num]->{$field->field_alias} = $field_output;
      }
    }
  }
  $vars['display_type'] = $view->granularity;
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
  $view->subtitle = theme('calendar_title', $view->granularity, $view);
  $view->mini = isset($view->mini) ? $view->mini : $view->granularity == 'year';
  
  // Create the calendar day names and rows.
  $rows = calendar_build_calendar($view, $items);
  $vars['items'] = $items;
  $vars['rows'] = $rows;
  
  $view->calendar_processed = TRUE;
  $vars['view'] = $view;  
  $vars['mini'] = !empty($view->mini);
  $vars['block'] = !empty($view->block);
}

/**
 * Display a month view.
 */
function template_preprocess_calendar_month(&$vars) {
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
  $view = $vars['view'];
  
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
  
  $vars['display_type'] = $view->granularity;
  $vars['min_date_formatted'] = date_format($view->min_date, DATE_FORMAT_DATETIME);
  $vars['max_date_formatted'] = date_format($view->max_date, DATE_FORMAT_DATETIME);
}

/**
 * Display a mini month view.
 */
function template_preprocess_calendar_mini(&$vars) {
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar_month($vars);
  $view = $vars['view'];
  $vars['show_title'] = $view->show_title;
}

/**
 * Display a year view.
 */
function template_preprocess_calendar_year(&$vars) {
  // Add in all the $vars added by the main calendar preprocessor.
  template_preprocess_calendar($vars);
  
  // Get the url of the year view and remove the year argument from it.
  // TODO clean this up in case there is another arg that looks like
  // the year to make sure only the year gets removed.
  $view = $vars['view'];
  $year = date_format($view->min_date, 'Y');
  $url = str_replace('/'. $year, '', $view->get_url());
  
  // Construct a calendar for each month, adjusting the $view passed 
  // to the theme so it will produce the right results.  
  $view = drupal_clone($vars['view']);
  $rows = $vars['rows'];
  $months = array();
  foreach ($rows as $month => $month_rows) {
    $view->month = $month;
    $view->granularity = 'month';
    $view->mini = TRUE;
    $view->hide_nav = TRUE;
    $view->with_weekno = FALSE;
    $view->show_title = TRUE;
    $view->url = $url;
    $view->min_date = date_make_date($view->year .'-'. date_pad($month) .'-01 00:00:00', date_default_timezone_name());
    $view->max_date = drupal_clone($view->min_date);
    date_modify($view->max_date, '+1 month');
    date_modify($view->max_date, '-1 second');
    $months[$month] = theme('calendar_mini', $view, $vars['options'], $month_rows);
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
  
  $vars['day_names'] = array();
  $view = $vars['view'];
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
 * Create the calendar date box.
 */
function template_preprocess_calendar_datebox(&$vars) {
  $date = $vars['date'];
  $view = $vars['view'];
  
  $vars['day'] = intval(substr($date, 8, 2));
  $vars['url'] = str_replace($view->date_arg, substr($date, 0, 10), $view->get_url());
  $vars['link'] = l($vars['day'], $vars['url']);
  $vars['granularity'] = $view->granularity;
  $vars['mini'] = $view->mini;
    
  if ($view->mini) {
    if (!empty($vars['selected'])) {
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
    $field_alias = $field->field_alias;
    if (!empty($node->$field_alias)) {
      $data = $node->$field_alias;
      $label = $field->options['label'];
      
      // CCK has some special label options.
      if (!empty($field->content_field)) {
        switch ($field->options['label_type']) {
          case 'none':
            $label = '';
            break;
          case 'widget':
            $label = $field->content_field['widget']['label'];
            break;
        }
      }
      if ($field_alias == $node->datefield) {
        $data = theme('calendar_date_combo', $node, $field->options['label'], $view);
      }
      $fields[$field_alias] = array(
        'id' => views_css_safe($field_alias),
        'label' => $label,
        'data' => $data,
        );
    }
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
  $node = $vars['node'];
  // Remote items may have a teaser to show.
  if (!empty($node->remote) && !empty($node->teaser)) {
    $fields['teaser'] = '<div class="content">'. ($node->teaser) ."</div>\n";
  }
}

/**
 * Preprocess an ical feed
 */
function template_preprocess_calendar_view_ical(&$vars) {
  global $base_url;
  global $language;
 
  $view     = &$vars['view'];
  $options  = &$vars['options'];
  $items    = &$vars['rows'];
  
  $style    = &$view->style_plugin;

  // Figure out which display which has a path we're using for this feed. If there isn't
  // one, use the global $base_url
  $link_display = $view->display_handler->get_link_display();

  // Compare the link to the default home page; if it's the default home page, just use $base_url.
  if (empty($vars['link'])) {
    $vars['link'] = $base_url;
  }

  // Keep devel module from appending queries to ical export.
  $GLOBALS['devel_shutdown'] = FALSE;
  
  drupal_set_header('Content-Type: text/calendar; charset=utf-8');
  drupal_set_header('Content-Disposition: attachment; filename="calendar.ics"; ');
  require_once('./'. drupal_get_path('module', 'date_api') .'/date_api_ical.inc');
  require_once('./'. drupal_get_path('module', 'date_api') .'/theme/theme.inc');
  require_once('./'. drupal_get_path('module', 'calendar') .'/calendar.inc');
  
  $events = array();
  foreach ($items as $node) {
    // Allow modules to affect item fields
    node_invoke_nodeapi($node, 'ical item');

    $event = array();
    $event['summary'] = strip_tags($node->{$view->summary_field});
    $event['start'] = $node->calendar_start_date;
    $event['end'] = $node->calendar_end_date;
    $event['location'] = !empty($node->{$view->location_field}) ? $node->{$view->location_field} : '';
    $event['description'] = !empty($node->{$view->description_field}) ? $node->{$view->description_field} : '';
    $event['uid'] = !empty($node->uid) ? $node->uid : (is_numeric($node->nid) ? url("node/$node->nid", array('absolute' => TRUE)) : $node->nid);
    $event['url'] = !empty($node->url) ? $node->url : (is_numeric($node->nid) ? url("node/$node->nid", array('absolute' => TRUE)) : $node->nid);
    $events[$event['uid']] = $event;
  }
  $vars['calname'] = variable_get('site_name', '') . (!empty($view->title) ? ' | '. $view->title : '');
  $vars['events'] = $events;
  template_preprocess_date_vcalendar($vars);
  
}

/**
 * Format an calendar week node for display.
 */
function template_preprocess_calendar_week_node(&$vars) {
  template_preprocess_calendar_node($vars);
}

/**
 * Format a node stripe legend
 */
function theme_calendar_stripe_legend() {
  if (empty($GLOBALS['calendar_stripes'])) {
    return '';
  }
  $header = array(
      array('class' => 'legend', 'data' => t('Item')),
      array('class' => 'legend', 'data' => t('Key'))
      );
  foreach ((array) $GLOBALS['calendar_stripes'] as $label => $stripe) {
    $rows[] = array($label, '<div style="background-color:'. $stripe .';color:'. $stripe .'" class="stripe" title="Key: '. $label .'">Key '. $label .'</div>');
  }
  $output = theme('table', $header, $rows, array('class' => 'mini legend'));
  return $output;
}

/**
 * Format node stripes
 * Add key value to text, then hide it with css for accessibility to screen readers
 */
function theme_calendar_stripe_stripe($node) {
  if (empty($node->stripe)) {
    return;
  }
  $GLOBALS['calendar_stripes'][$node->stripe_label] = $node->stripe;
  return '<div style="background-color:'. $node->stripe .';color:'. $node->stripe .'" class="stripe" title="Key: '. $node->stripe_label .'">Key '. $node->stripe_label .'</div>'."\n";
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
 * Theme the calendar title
 */
function theme_calendar_title($granularity, $view) {
  switch ($granularity) {
    case 'year':
      return $view->year;
    case 'month':
      return date_format_date($view->min_date, 'custom', 'F');
    case 'day':
      return date_format_date($view->min_date, 'custom', 'l, F j Y');
    case 'week':
    	return t('Week of @date', array('@date' => date_format_date($view->min_date, 'custom', 'F j')));
  }
}

/**
 * Format a from/to date in the calendar view.
 *
 * Alter the display as appropriate for the type of view.
 * We can fine-tune the display to show only the time in 
 * the calendar month and week cells but the
 * whole date in other places.
 */
function theme_calendar_date_combo($node, $label, $view) {
  switch ($view->style_plugin->definition['handler']) {
    // Some of the calendar views need a smaller date format.
    case 'calendar_view_plugin_style':
      switch ($view->calendar_display) {
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
  $date1 = theme('date_all_day', 'date1', $node->calendar_start_date, $node->calendar_end_date, $format, $node, $view);
  $date2 = theme('date_all_day', 'date2', $node->calendar_start_date, $node->calendar_end_date, $format, $node, $view);
  
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