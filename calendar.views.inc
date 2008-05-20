<?php
//$Id$
/**
 * @file
 * 
 * Creates a generic Views date argument that can be used with any style,
 * and a Calendar plugin style that uses that argument to render the results
 * in a year/month/day/week calendar, with back/next navigation links.
 * 
 */
views_include_handlers();

/**
 * Implementation of hook_views_data()
 */
function calendar_views_data() {
  $data = array();
  $data['node']['calendar_date'] = array(
    'group' => t('Calendar'),
    'title' => t('Date'),
    'help' => t('Filter any Views date field by a date argument, using any common ISO format (i.e. YYYY, YYYY-MM, YYYY-MM-DD, or YYYY-W99).'),
    'argument' => array(
      'handler' => 'calendar_argument_date',
      'calendar_type' => 'date',
      'type' => 'calendar',
      'empty name field' => t('Undated'),
      'arg_format' => 'Y-m',
    ),
  );  
  $data['user'] = $data['node'];
  $data['comment'] = $data['node'];
  return $data;
}

/**
 * Implementation of hook_views_plugins
 */
function calendar_views_plugins() {
  $path = drupal_get_path('module', 'calendar');
  require_once "./$path/theme/theme.inc";

  $base = array(
    'file' => 'theme.inc',
    'path' => "$path/theme",
  );
  return array(
    'module' => 'calendar', // This just tells our themes are elsewhere.
    'style' => array(
      'calendar' => array(
        'title' => t('Calendar'),
        'help' => t('Displays Views results in a multi-dimensional calendar, with back/next navigation.'),
        'handler' => 'calendar_plugin_style',
        'theme' => 'calendar_month',
        'file' => 'theme.inc',
        'path' => "$path/theme",
        'additional themes' => array(
          'calendar_year' => 'style',
          'calendar_day' => 'style',
          'calendar_week' => 'style',
          ),
        'uses row plugin' => TRUE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
    'row' => array(
      'calendar' => array(
        'title' => t('Calendar'),
        'help' => t('Displays an individual calendar item, with all its fields.'),
        'handler' => 'calendar_node',
        'theme' => 'calendar_node',
        'path' => "$path/theme",
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
      ),
    ),
    // Add an option to set a default value for an empty date argument.
    'argument default' => array(
      'calendar' => array(
        'title' => t('Current date'),
        'handler' => 'calendar_plugin_argument_default',
      ),
    ),
  );
}

/**
 * Calendar argument handler.
 * 
 * @param $arg_format
 *   The format string to use on the current time when 
 *   creating a default date argument.
 * 
 */
class calendar_argument_date extends views_handler_argument_formula {
  var $arg_format = 'Y-m';
  
  function construct() {
    parent::construct();
    $this->arg_format = $this->definition['arg_format'];
  }
  
  // Update the summary values to show selected granularity.
  function admin_summary() {
    if (!empty($this->options['date_fields'])) {
      return ' ('. implode(', ', $this->options['date_fields']) .')';
    }
    else {
      return parent::admin_summary();
    }
  }
    
  /**
   * Default value for the date_fields option.
   */
  function options(&$options) {
    parent::options($options);
    $options['date_fields'] = array();
    $options['date_method'] = 'OR';
  }

  /**
   * Add a form element to select date_fields for this argument.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $fields = calendar_fields();
    foreach ($fields as $name => $field) {
      $options[$name] = $field['label'];
    }
    $form['date_fields'] = array(
      '#title' => t('Date field(s)'),
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => $this->options['date_fields'],
      '#multiple' => TRUE,
      '#description' => t('Select one or more date fields to filter with this argument.'),
    );
    $form['date_method'] = array(
      '#title' => t('Method'),
      '#type' => 'radios',
      '#options' => array('OR' => t('OR'), 'AND' => t('AND')),
      '#default_value' => $this->options['date_method'],
      '#description' => t('Method of handling multiple date fields in the same query. Return items that have any matching date field (date = field_1 OR field_2), or only those with matches in all selected date fields (date = field_1 AND field_2).'),
      );
    
  }
    
  function options_validate($form, &$form_state) {
    if (empty($form_state['values']['options']['date_fields'])) {
      form_error($form, t('You must select at least one date field for this argument.'));
    }
  }
  
  function options_submit($form, &$form_state) {
    $form_state['values']['options']['date_fields'] = array_filter($form_state['values']['options']['date_fields']);
  }
  
  /**
   * Set up the query for this argument.
   *
   * The argument sent may be found at $this->argument.
   */
  function query() {
    include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
    include_once('./'. drupal_get_path('module', 'calendar') .'/calendar.inc');
  
    $this->ensure_my_table();
    
    $this->arg_format = calendar_arg_format($this->argument);
    $parts = calendar_arg_parts($this->argument);

    foreach ($parts as $key => $part) {
      // The last part evaluated is the one that will 'stick'
      // as the calendar type.
      $this->calendar_type = $key;
      $this->$key = $part;
    }

    $now = date_now();
    if ($this->calendar_type == 'week' && calendar_part_is_valid($this->week, 'week')) {
      $range = calendar_week_range($this);
      $min_date = $range[0];
      $max_date = $range[1];
    
    } else {
      $month = calendar_part_is_valid($this->month, 'month') ? $this->month : 1;
      $day   = calendar_part_is_valid($this->day, 'day') ? $this->day : 1;
      $year  = calendar_part_is_valid($this->year, 'year') ? $this->year : date_format($now, 'Y');
      $min_date  = date_create($year .'-'. date_pad($month) .'-'. date_pad($day) .' 00:00:00', date_default_timezone());
      $max_date = drupal_clone($min_date);
      date_modify($max_date, '+1 '. $this->calendar_type);
      date_modify($max_date, '-1 second');
    }
    
    // Create min and max dates in both local and UTC time.
    // We'll compare fields to the UTC date whenever possible
    // to avoid the need to do timezone conversions. When that
    // isn't possible (the date is not stored in UTC or needs to
    // be converted back to a time that may be different than
    // the local timezone) we will have to do tz conversions in
    // the database.
    $this->min_date = $min_date;
    $this->min_utc = drupal_clone($min_date);
    date_timezone_set($this->min_utc, timezone_open('UTC'));
    $this->max_date = $max_date;
    $this->max_utc = drupal_clone($max_date);
    date_timezone_set($this->max_utc, timezone_open('UTC'));
        
    // Use set_where_group() with the selected date_method 
    // of 'AND' or 'OR' to create the where clause.
    $fields = calendar_fields();
    $this->query->set_where_group($this->options['date_method'], 'calendar_date');
    foreach ($this->options['date_fields'] as $delta => $name) {
      if ($field = $fields[$name]) {
        static $date_handler = array();
        
        // Set up a date handler for each field, and store it in a
        // static variable so we don't re-do the work for every field.
        if (!array_key_exists($name, $date_handler)) {
          $date_handler[$name] = new date_sql_handler();
          $date_handler[$name]->construct($field['sql_type'], date_default_timezone_name());
          $tz_handling  = $field['tz_handling'];
          switch ($tz_handling) {
            case 'date' :
              $date_handler[$name]->db_timezone = 'UTC';
              $date_handler[$name]->local_timezone_field = $fields['tz_field'];
              $date_handler[$name]->local_offset_field = $fields['offset_field'];
              $date_handler[$name]->min_date = $this->min_date;
              $date_handler[$name]->max_date = $this->max_date;
              break;
            case 'none':
              $date_handler[$name]->db_timezone = date_default_timezone_name();
              $date_handler[$name]->local_timezone = date_default_timezone_name();
              $date_handler[$name]->min_date = $this->min_date;
              $date_handler[$name]->max_date = $this->max_date;
              break;
            case 'utc':
              $date_handler[$name]->db_timezone = 'UTC';
              $date_handler[$name]->local_timezone = 'UTC';
              $date_handler[$name]->min_date = $this->min_utc;
              $date_handler[$name]->max_date = $this->max_utc;
              break;
            default :
              $date_handler[$name]->db_timezone = 'UTC';
              $date_handler[$name]->local_timezone = 'UTC';
              $date_handler[$name]->min_date = $this->min_utc;
              $date_handler[$name]->max_date = $this->max_utc;
              break;
          }
        }
        // Make sure this field is added to the query.
        $this->query->add_field($field['table_name'], $field['field_name']);
        $from = $date_handler[$name]->sql_where_date('DATE', $field['fullname'], '>=', date_format($date_handler[$name]->min_date, DATE_FORMAT_DATETIME));
        $to   = $date_handler[$name]->sql_where_date('DATE', $field['fullname'], '<=', date_format($date_handler[$name]->max_date, DATE_FORMAT_DATETIME));
        $sql = str_replace('***table***', $this->table_alias, "($from AND $to)");
        if ($sql) {
          $this->query->add_where('calendar_date', $sql);
        }
      }
    }
  }
}

/**
 * Default argument plugin to default to the current date.
 */
class calendar_plugin_argument_default extends views_plugin_argument_default {
  var $option_name = 'default_argument_calendar';
  
  function argument_form(&$form, &$form_state) {
    $form[$this->option_name] = array(
      '#title' => t('Current date format'),
      '#description' => t('Select a format to use when creating a missing argument from the current date.'),
      '#type' => 'select',
      '#options' => array('Y-m-d' => 'YYYY-MM-DD', 'Ymd' => 'YYYYMMDD', 
        'Y-m' => 'YYYY-MM', 'Ym' => 'YYYYMM', 'Y' => 'YYYY', 
        'Y-\Ww' => 'YYYY-W99', 'Y\Ww' => 'YYYYW99'),
      '#default_value' => $this->format(),
      '#process' => array('views_process_dependency'),
      '#dependency' => array(
        'radio:options[default_action]' => array('default'),
        'radio:options[default_argument_type]' => array($this->id)
      ),
      '#dependency_count' => 2,
    );
    $this->check_access($form);
  }
  
  function format() {
    return !empty($this->argument->options[$this->option_name]) ? $this->argument->options[$this->option_name] : 'Y-m';
  }
  
  function get_argument() {
    return date($this->format(), time());
  }
}

/**
 * Style plugin to render each item in a calendar.
 */
class calendar_plugin_style extends views_plugin_style {
  
  /**
   * Set default options
   */
  function options(&$options) {
    $options['calendar_time_format'] = 'H:i';
    $options['calendar_day_header'] = 1;
    $options['date_fields'] = array();
    $options['block_args'] = '';
    $options['default_display'] = 'month';
    foreach (calendar_display_types() as $display => $name) {
      $options[$display] = 'calendar';
    }
  }

  function display_type_options() {
    return array(
      'calendar' => t('Calendar'), 
      'table' => t('Table'), 
      'teasers' => t('Teasers'), 
      'nodes' => t('Full Nodes'), 
      'list' => t('List'), 
      '' => t('None'));
  }
  
  /**
   * Calendar argument date fields used in this view.
   */
  function date_fields() {
    $date_fields = array();
    $calendar_fields = calendar_fields();
    $arguments = $this->display->handler->get_option('arguments');
    foreach ($arguments as $name => $argument) {
      if (isset($argument['date_fields'])) {
        foreach ($argument['date_fields'] as $date_field) {
          $field = $calendar_fields[$date_field];
          $fullname = "{$field['table_name']}.{$field['field_name']}";
          $handler = views_get_handler($field['table_name'], $field['field_name'], 'field');
          if ($handler) {
            $date_fields[$fullname] = $field;
            $date_fields[$fullname]['name'] = $handler->ui_name();
          }
        }
      }
    }
    return ($date_fields);
  }
  
  /**
   * Style validation.
   */
  function validate() {
    $errors = parent::validate();
    
    $arguments = $this->display->handler->get_option('arguments');
    if (!in_array('calendar_date', array_keys($arguments))) {
      $errors[] = t('Style @style requires the Calendar: Date argument.', array('@style' => $this->definition['title']));      
    }
      
    // TODO fix the following validation code to work correctly in Views2.
    // Make sure all arguments are set to 'Display all values'.
    //$arg_types = array();
    //$cal_args = calendar_args();
    //foreach ($view['argument'] as $delta => $argument) {
    //  if (in_array($argument['id'], $cal_args)) {
    //    $view['argument'][$delta]['argdefault'] = 2;
    //    if ($argument['argdefault'] != 2) {
          //form_error($form['argument'][$delta]['argdefault'], t('Calendar arguments must be set to \'Display All Values\'.'));
    //    }
    //    $arg_types[] = $argument['id'];
    //  }
    //}
  
    // CCK date fields cannot use grouped handler.
    //$calendar_fields = array_keys(calendar_fields());
    //foreach ($view['field'] as $delta => $field) {
    //  if (in_array($field['field_name'], $calendar_fields) && $field['handler'] == 'content_views_field_handler_group') {
        //form_error($form['field_name'][$delta]['handler'], t('Calendar CCK Date fields must be set to \'Do not group multiple values\'.'));
    //  }
    //}
    return $errors;
  }

  /**
   * Style options.
   */
  function options_form(&$form, &$form_state) {
    $arguments = $this->display->handler->get_option('arguments');
    if (!in_array('calendar_date', array_keys($arguments))) {
      $form['error_markup'] = array(
        '#value' => t('You need to add a Calendar: Date argument before you can configure your calendar settings'),
        '#prefix' => '<div class="error form-item description">',
        '#suffix' => '</div>',
      );
      return;
    }

    $options = array();
    $date_fields = $this->date_fields();
    foreach ($date_fields as $field => $info) {
      $options[$field] = $info['name'];
    }
    
    $form['date_fields'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Date field(s)'),
      '#options' => $options,
      '#default_value' => drupal_map_assoc(array_keys($options)),
      '#description' => t('The date fields which will be displayed in the calendar. Date fields are selected in the Calendar: Date argument.'),
      '#attributes' => array('disabled' => TRUE),
    );
    
    $form['default_display'] = array(
      '#title' => t('Default display'),
      '#description' => t('The display that users will see if no arguments have been selected.'),
      '#default_value' => $this->options['default_display'],
      '#type' => 'select',
      '#options' => calendar_display_types(),
    );
    $form['year'] = array(
      '#title' => t('Year display'),
      '#default_value' => $this->options['year'],
      '#type' => 'select',
      '#options' => $this->display_type_options(),
    );
    $form['month'] = array(
      '#title' => t('Month display'),
      '#default_value' => $this->options['month'],
      '#type' => 'select',
      '#options' => $this->display_type_options(),
    );
    $form['week'] = array(
      '#title' => t('Week display'),
      '#default_value' => $this->options['week'],
      '#type' => 'select',
      '#options' => $this->display_type_options(),
    );
    $form['day'] = array(
      '#title' => t('Day display'),
      '#default_value' => $this->options['day'],
      '#type' => 'select',
      '#options' => $this->display_type_options(),
    );
    $time = mktime(1, 15, 0, 1, 1, date('Y', time()));
    $time_options = array(
      'G:i' => date('G:i', $time),     'g:ia' => date('g:ia', $time),
      'g:iA' => date('g:iA', $time),   'g:i a' => date('g:i a', $time),
      'g:i A' => date('g:i A', $time), 'H:i' => date('H:i', $time),
      'h:ia' => date('h:ia', $time),   'h:iA' => date('h:iA', $time),
      'h:i a' => date('h:i a', $time), 'h:i A' => date('h:i A', $time),
      );
    $form['calendar_time_format'] = array(
      '#title' => t('Time format'),
      '#default_value' => $this->options['calendar_time_format'],
      '#type' => 'select',
      '#options' => $time_options,
      '#description' => t('The format to use for the time-only date display.'),
      );
    $form['calendar_day_header'] = array(
      '#title' => t('Mini day name size'),
      '#default_value' => $this->options['calendar_day_header'],
      '#type' => 'select',
      '#options' => drupal_map_assoc(range(1, 3)),
      '#description' => t('The number of letters to show for day of week names in the mini calendar.'),
      );
    $form['block_args'] = array(
      '#title' => t('Argument type'),
      '#default_value' => $this->options['block_args'],
      '#type' => 'select',
      '#options' => array('' => t('Page'), 'mini' => t('Block')),
      '#description' => t('Page arguments will derive argument information from the page url. Block arguments will derive argument information from a query string like \'&mini=2008-05\', so it can be used independently of the page url.'),
    );
  }
  
  function options_submit($form, &$form_state) {
    $form_state['values']['style_options']['date_fields'] = array_keys($form_state['values']['style_options']['date_fields']);
  }
  
  function query() {
    include_once(drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
        
    $style_options = $this->view->style_plugin->options;
    
    // Evaluate our argument values and figure out which 
    // calendar display we need to create.
    $i = 0;    
    foreach ($this->view->argument as $delta => $argument) {
      if ($argument['id'] == 'calendar_date') {
        $this->view->calendar_type = $argument['handler']->calendar_type;
        $this->view->calendar_arg = $argument['handler']->argument;
        $this->view->calendar_arg_pos = $i;
        $this->view->year = $argument['handler']->year;
        $this->view->month = $argument['handler']->month;
        $this->view->day = $argument['handler']->day;
        $this->view->week = $argument['handler']->week;
        $this->view->min_date = $argument['handler']->min_date;
        $this->view->max_date = $argument['handler']->max_date;
      }
      $i++;
    }
     
    // Identify the kind of display we're using for this view.
    // Check first for 'view' in url to get displays set by the switch
    // block.
    $style_options = $this->view->style_plugin->options;
    if (isset($_GET['view'])) {
  	  $this->view->calendar_display = !empty($_GET['view']) ? check_plain($_GET['view']) : 'calendar';
    }
    else {
  	  $this->view->calendar_display = $style_options[$this->view->calendar_type];
    }
        
    // bring the node type into the query so we can use it in the theme
    $this->view->query->add_field('node', 'type');
    
    parent::query();

  }
  
  /**
   * Render the calendar style.
   */
  function render() {
    
    // TODO is this the best way to identify if we need the mini calendar?
    
    if ($this->view->display_handler->definition['handler'] == 'views_plugin_display_block') {
      $this->view->mini = TRUE;
    }
    
    // Adjust the theme to match the currently selected calendar type.
    
    $this->definition['theme'] = 'calendar_'. $this->view->calendar_type;
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}

/**
 * The calendar row plugin
 *
 * No options set up at this time, 
 * exists to provide a template for the output.
 */
class calendar_node extends views_plugin_row {

}

