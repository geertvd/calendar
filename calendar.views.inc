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
      'arg_format' => 'Y-m-d',
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
  return array(
    'module' => 'calendar', // This just tells our themes are elsewhere.
    'style' => array(
      'calendar' => array(
        'title' => t('Calendar'),
        'help' => t('Displays Views results in a multi-dimensional calendar, with back/next navigation.'),
        'handler' => 'views_plugin_style_calendar',
        'theme' => 'calendar_views_calendar',
        'uses row plugin' => TRUE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
      ),
    ),
    // Add an option to set a default value for an empty date argument.
    'argument default' => array(
      'calendar' => array(
        'title' => t('Current date'),
        'handler' => 'calendar_plugin_argument_calendar',
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
  var $arg_format = 'Y-m-d';
  
  function construct() {
    parent::construct();
    $this->arg_format = $this->definition['arg_format'];
  }
  
  // Update the summary values to show selected granularity.
  function admin_summary() {
    return ' ('. implode(', ', $this->options['date_fields']) .')';
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
    if (empty($form_state['value']['options']['date_fields'])) {
      $form_error($form, t('You must select at least one date field for this argument.'));
    }
  }
  
  function options_submit($form, &$form_state) {
    $form_state['values']['options']['date_fields'] = array_filter($form_state['values']['options']['date_fields']);
  }
  
  function build_sql($name, $argument) {
    $fields = calendar_fields();
    if ($field = $fields[$name]) {
      // Make sure this field is added to the query.
      $this->query->add_field($field['table'], $field['field_name']);
      
      $this->date_handler->date_type = $field['sql_type'];
      $sql = $this->date_handler->sql_where_format($this->arg_format, $field['fullname'], '=', $argument);
      $sql = str_replace('***table***', $this->table_alias, $sql);
      return $sql;
    }
    return FALSE;
  }
  
  /**
   * Set up the query for this argument.
   *
   * The argument sent may be found at $this->argument.
   */
  function query() {
    include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
    
    // If this argument is in a display using the calendar style plugin,
    // we want to skip creating the query here and do it in the style
    // instead. Not sure how to do that since this argument seems to have
    //  no information about what kind of display it is attached to.
    
    $this->ensure_my_table();
    $this->date_handler = new date_sql_handler();
    $this->date_handler->construct($this_field['type'], date_default_timezone_name());
    
    $tz_handling  = $this_field['tz_handling'];
    switch ($tz_handling) {
      case 'date' :
        $date_handler->db_timezone = 'UTC';
        $date_handler->local_timezone_field = $fields['tz_field'];
        $date_handler->local_offset_field = $fields['offset_field'];
        break;
      // 'None' doesn't actually use UTC, but we've set the db timezone
      // to UTC so setting the local timezone to UTC will accomplish
      // what we want to accomplish - do no timezone conversion.
      case 'none':
      case 'utc':
        $date_handler->db_timezone = 'UTC';
        $date_handler->local_timezone = 'UTC';
        break;
      default :
        $date_handler->db_timezone = 'UTC';
        $date_handler->local_timezone = date_default_timezone_name();
        break;
    }

    $this->arg_format = calendar_arg_format($this->argument);

    // Use set_where_group() with the selected date_method 
    // of 'AND' or 'OR' to create the where clause.
    $this->query->set_where_group($this->options['date_method'], 'calendar_date');
    foreach ($this->options['date_fields'] as $delta => $name) {
      if ($sql = $this->build_sql($name, $this->argument)) {
        $this->query->add_where('calendar_date', $sql);
      }
    }
  }
}

/**
 * Default argument plugin to default to the current date.
 */
class calendar_plugin_argument_calendar extends views_plugin_argument_default {
  var $option_name = 'default_argument_calendar';
  
  function argument_form(&$form, &$form_state) {
    $form[$option_name] = array(
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
class views_plugin_style_calendar extends views_plugin_style {
  
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
          $handler = views_get_handler($field['table'], $field['field'], 'field');
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
    //  if (in_array($field['field'], $calendar_fields) && $field['handler'] == 'content_views_field_handler_group') {
        //form_error($form['field'][$delta]['handler'], t('Calendar CCK Date fields must be set to \'Do not group multiple values\'.'));
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
    if (empty($this->options['date_fields'])) {
      $this->options['date_fields'] = array();
    }

    $form['date_fields'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Date field(s)'),
      '#options' => $options,
      '#default_value' => array_keys($options),
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
    $form_state['values']['options']['date_fields'] = array_filter($form_state['values']['options']['date_fields']);
  }
  
  function query() {
    include_once(drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
        
    $this->view->real_url = calendar_real_url($this->view, $this->view->args);
    $style_options = $this->view->style_handler->options;
    
    $this->view->real_args = $this->view->args;
    
    // Replace real url with block url, if requested.
    if (!empty($style_options['block_args'])) {
      $this->view->args = explode('/', str_replace($this->view->url .'/', '', $_GET[$style_options['block_args']]));
    }
    
    // Evaluate our argument values and figure out which 
    // calendar display we need to create.
    $i = 0;    
    foreach ($this->view->argument as $delta => $argument) {
      if ($argument['id'] == 'calendar_date') {
        $pos = $i;
        $parts = calendar_arg_parts($this->view->args[$pos]);
      }
      $i++;
    }
    // If there is no calendar argument, set a default value.
    if (empty($parts)) {
      switch ($this->options['default_display']) {
        case 'year':
          $parts = array('year' => date('Y'));
          break;
        case 'day':
          $parts = array('year' => date('Y'), 'month' => date('m'), 'day' => date('d'));
          break;
        case 'week':
          $parts = array('year' => date('Y'), 'week' => date('\Ww'));
          break;
        default:
          $parts = array('year' => date('Y'), 'month' => date('m'));
      }
    }
    foreach ($parts as $key => $part) {
      // The last part evaluated is the one that will 'stick'
      // as the calendar type.
      $this->view->calendar_type = $key;
      $this->view->$key = $part;
    }
    
    // Identify the kind of display we're using for this view.
    // Check first for 'view' in url to get displays set by the switch
    // block.
    $style_options = $this->view->style_handler->options;
    if (isset($_GET['view'])) {
  	  $this->view->calendar_display = !empty($_GET['view']) ? check_plain($_GET['view']) : 'calendar';
    }
    else {
  	  $this->view->calendar_display = $style_options[$this->view->calendar_type];
    }
        
    $now = date_now();
    if ($this->view->calendar_type == 'week' && calendar_part_is_valid($this->view->week, 'week')) {
      $range = calendar_week_range($this->view);
      $date = $range[0];
      $max_date = $range[1];
    
    } else {
      $month = calendar_part_is_valid($this->view->month, 'month') ? $this->view->month : 1;
      $day   = calendar_part_is_valid($this->view->day, 'day') ? $this->view->day : 1;
      $year  = calendar_part_is_valid($this->view->year, 'year') ? $this->view->year : date_format($now, 'Y');
      $date  = date_create($year .'-'. date_pad($month) .'-'. date_pad($day) .' 00:00:00', date_default_timezone());
      $max_date = drupal_clone($date);
      date_modify($max_date, '+1 '. $this->view->calendar_type);
      date_modify($max_date, '-1 second');
    }
    $this->view->min_date = $date;
    $this->view->max_date = $max_date;
        
    // bring the node type into the query so we can use it in the theme
    $this->view->query->add_field('node', 'type');
    if ($queries) $this->view->query->add_where(implode(" OR ", $queries));
    parent::query();

  }
  
  /**
   * Render the table style.
   */
  function render() {
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}

/**
 * Display a view as a table style.
 */
function template_preprocess_calendar_views_calendar(&$vars) {
  $view     = $vars['view'];
  $result   = $view->result;
  $options  = $view->style_handler->options;
  $handler  = $view->style_handler;

  $fields   = $view->field;
  $vars['result'] = $view->result;
  
  $query    = tablesort_get_querystring();
  if ($query) {
    $query = '&' . $query;
  }
  
  $vars['display_type'] = $view->calendar_type;
  $vars['min_date'] = date_format($view->min_date, 'r');
  $vars['max_date'] = date_format($view->max_date, 'r');
  
  //$vars['header'] = calendar_nav($view);
  
  // Render each field into its appropriate column.
  foreach ($result as $num => $row) {
    foreach ($fields as $field) {
      if (!empty($field['handler']) && is_object($field['handler'])) {
        $handler = &$field['handler'];
        $field_output = $handler->theme($row);
        $vars['rows'][$num] .= $field_output;
      }
    }
  }
}