<?php
//$Id$
views_include_handlers();

/**
 * Implementation of hook_views_data()
 */
function calendar_views_data() {
  $data = array();
  $data['node']['calendar_year'] = array(
    'group' => t('Calendar'),
    'title' => t('Year'),
    'help' => t('Filter date fields by the calendar year (YYYY). When using the Calendar style plugin, you will need to add Calendar: Year, Calendar: Month, and Calendar: Day arguments, in that order, to the view.'),
    'argument' => array(
      'handler' => 'calendar_argument_date',
      'calendar_type' => 'year',
      'type' => 'calendar',
      'numeric' => TRUE,
      'empty name field' => t('Undated'),
      'arg_format' => 'Y',
    ),
  );  
  $data['node']['calendar_month'] = array(
    'group' => t('Calendar'),
    'title' => t('Month'),
    'help' => t("Filter date fields by the calendar month (1-12). When using the Calendar style plugin, you will need to add Calendar: Year, Calendar: Month, and Calendar: Day arguments, in that order, to the view."),
    'argument' => array(
      'handler' => 'calendar_argument_date',
      'calendar_type' => 'month',
      'type' => 'calendar',
      'numeric' => TRUE,
      'empty name field' => t('Undated'),
      'arg_format' => 'm',
    ),
  );  
  $data['node']['calendar_day'] = array(
    'group' => t('Calendar'),
    'title' => t('Day'),
    'help' => t("Filter date fields by the calendar day (1-31). When using the Calendar style plugin, you will need to add Calendar: Year, Calendar: Month, and Calendar: Day arguments, in that order, to the view."),
    'argument' => array(
      'handler' => 'calendar_argument_date',
      'calendar_type' => 'day',
      'type' => 'calendar',
      'numeric' => TRUE,
      'empty name field' => t('Undated'),
      'arg_format' => 'd',
    ),
  );  
  $data['node']['calendar_week'] = array(
    'group' => t('Calendar'),
    'title' => t('Week'),
    'help' => t("Filter date fields by the week number (W1-W52). Use a 'W' in front of the week number in the url. When using the Calendar style plugin, skip this argument, it will automatically be added to the view when needed."),
    'argument' => array(
      'handler' => 'calendar_argument_date',
      'calendar_type' => 'week',
      'type' => 'calendar',
      'numeric' => TRUE,
      'empty name field' => t('Undated'),
      'arg_format' => 'w',
    ),
  );  
  return $data;
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
  var $option_name = 'default_argument_calendar';
  var $arg_format = 'Y-m-d';
  
  function construct() {
    parent::construct();
    $this->arg_format = $this->definition['arg_format'];
  }
  
  /**
   * Add an option to set the default value to the current date.
   */
  function default_argument_form(&$form, &$form_state) {
    parent::default_argument_form($form, $form_state);
    $form['default_argument_type']['#options'] += array('calendar' => t('Current @title', array('@title' => $this->definition['title'])));
  }
  
  /**
   * Set the empty argument value to the current date, 
   * formatted appropriately for this argument.
   */
  function get_default_argument($raw = FALSE) {
    if (!$raw && $this->options['default_argument_type'] == 'calendar') {
      return date($this->arg_format, time());
    }
    else {
      return parent::get_default_argument($raw);
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
      '#description' => t('Select one or more date fields to filter by @title. <strong>All Calendar arguments in the same view should use the same date field(s).</strong>', array('@title' => $this->definition['title'])),
    );
    $form['date_method'] = array(
      '#title' => t('Method'),
      '#type' => 'radios',
      '#options' => array('OR' => t('Match any selected date field'), 'AND' => t('Match all selected date fields')),
      '#default_value' => $this->options['date_method'],
      '#description' => t('Method of handling multiple date fields in the same query.'),
      );
    parent::options_form($form, $form_state);
  }
    
  function options_submit($form, &$form_state) {
    $form_state['values']['options']['date_fields'] = array_filter($form_state['values']['options']['date_fields']);
  }
  
  function get_formula() {
    include_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
    $this->date_handler = new date_sql_handler();
    $this->date_handler->construct();
    $this->date_handler->local_timezone = date_default_timezone_name();

    $fields = calendar_fields();
    $formula = array();
    foreach ($this->options['date_fields'] as $delta => $name) {
      if ($field = $fields[$name]) {
        // Make sure this field is added to the query.
        $this->query->add_field($field['table'], $field['field_name']);
        // Construct a formula array, using an array in case there is 
        // more than one date field.
        $this->date_handler->date_type = $field['sql_type'];
        $sql_field = $this->date_handler->sql_field($field['fullname']);
        $sql_field = $this->date_handler->sql_format($this->arg_format, $sql_field);
        $sql = str_replace('***table***', $this->table_alias, $sql_field);
        $formula[] = $sql;
      }
    }
    $this->formula = $formula;
    return $formula;
  }
    
  /**
   * Set up the query for this argument.
   *
   * The argument sent may be found at $this->argument.
   */
  function query() {
    $this->ensure_my_table();
    $formula = $this->get_formula();
    $placeholder = empty($this->definition['numeric']) ? "'%s'" : '%d';
    $queries = array();
    foreach ($formula as $field) {
      $queries[] = "$field = $placeholder";
      $args[] = $this->argument;
    }
    $this->query->add_where(0, '('. implode(") {$this->options['date_method']} (", $queries) .')', $args);
  }
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
        'help' => t('Displays rows in a calendar.'),
        'handler' => 'views_plugin_style_calendar',
        'theme' => 'calendar_views_calendar',
        'uses row plugin' => TRUE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
      ),
    ),
  );
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
    foreach ($this->display_types() as $display) {
      $options[$display] = 'calendar';
    }
  }

  function display_types() {
    return array('year', 'month', 'day', 'week', 'block');  
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
      if (in_array($name, calendar_args())) {
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
    $cal_args = calendar_args();
    if (sizeof(array_intersect(array_keys($arguments), $cal_args)) < 3) {
      $errors[] = t('Style @style requires Calendar Year, Month, and Day arguments.', array('@style' => $this->definition['title']));
    }
  
    // TODO fix the following validation code to work correctly in Views2.
    // Make sure all arguments are set to 'Display all values'.
    //$arg_types = array();
    //$cal_args = calendar_args();
    //foreach ($view['argument'] as $delta => $argument) {
    //  if (in_array($argument['type'], $cal_args)) {
    //    $view['argument'][$delta]['argdefault'] = 2;
    //    if ($argument['argdefault'] != 2) {
          //form_error($form['argument'][$delta]['argdefault'], t('Calendar arguments must be set to \'Display All Values\'.'));
    //    }
    //    $arg_types[] = $argument['type'];
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
    $cal_args = calendar_args();
    if (sizeof(array_intersect(array_keys($arguments), $cal_args)) < 3) {
      $form['error_markup'] = array(
        '#value' => t('You need to add Calendar Year, Month, and Day arguments before you can configure your calendar settings'),
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
      '#description' => t('The date fields which will be displayed in the calendar. Date fields are selected in the Calendar arguments. All Calendar arguments should use the same date fields!'),
      '#attributes' => array('disabled' => TRUE),
    );
    
    $time = mktime(1, 15, 0, 1, 1, date('Y', time()));
    $time_options = array(
      'G:i' => date('G:i', $time),
      'g:ia' => date('g:ia', $time),
      'g:iA' => date('g:iA', $time),
      'g:i a' => date('g:i a', $time),
      'g:i A' => date('g:i A', $time),
      'H:i' => date('H:i', $time),
      'h:ia' => date('h:ia', $time),
      'h:iA' => date('h:iA', $time),
      'h:i a' => date('h:i a', $time),
      'h:i A' => date('h:i A', $time),
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
      '#description' => t('The number of letters to show for mini calendar day of week names.'),
      );

    $form['display'] = array(
      '#type' => 'markup',
      '#value' => t('<div class="form-item"><strong>Display Options:</strong><br />Choose the way the calendar entries should be displayed. Selecting \'None\' will hide links to that option.</div>'),
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
    $form['block'] = array(
      '#title' => t('Block display'),
      '#default_value' => $this->options['block'],
      '#type' => 'select',
      '#options' => $this->display_type_options(),
    );
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
  
  $active   = !empty($handler->active) ? $handler->active : '';
  $order    = !empty($handler->order) ? $handler->order : 'asc';

  $query    = tablesort_get_querystring();
  if ($query) {
    $query = '&' . $query;
  }

  $vars['header'] = calendar_nav($this);
  
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