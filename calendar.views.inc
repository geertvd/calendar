<?php
//$Id$
/**
 * @file
 * 
 * Creates calendar displays of Views results.
 * 
 * Unlike previous versions of the Calendar module, there is just a single
 * Date argument instead of year, month, and day arguments. The argument
 * value will be YYYY-MM-DD for a day, YYYY-MM for a month, YYYY for a
 * year, and YYYY-W99 for a week. There is a default option to set the 
 * argument to the current date when the argument is empty.
 * 
 * A calendar display creates calendar navigation and links to 
 * multiple displays for the year, month, day, or week views. The actual
 * displays are created by attaching calendar views that use whatever
 * styles are desired for those pages. The calendar display requires the 
 * calendar argument to do its work and uses its own style plugin 
 * and template.
 * 
 * Calendar views are attachments to create the year, month, day,
 * and week displays. They can be set to use any style, either a
 * calendar style or any other Views style, like teasers or lists.
 * They must be attached to the main calendar page or to a calendar
 * block.
 * 
 * A calendar block will create a calendar block for the
 * view results. This block requires the calendar argument to do
 * its work, and uses one of the year, month, day, or week calendar 
 * view attachments created above.
 * 
 */
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
    'display' => array(
      // Main calendar display plugin.
      'calendar' => $base + array(
        'title' => t('Calendar'),
        'help' => t('This is the main calendar page. Add calendar views for the year, month, day, and week displays and attach them to this page.'),
        'handler' => 'calendar_plugin_display_page',
        'theme' => 'views_view',
        'uses hook menu' => TRUE,
        'use ajax' => TRUE,
        'use pager' => FALSE,
        'accept attachments' => TRUE,
        'admin' => t('Calendar navigation'),
        'help topic' => 'display-calendar',
      ),
      // Calendar block display plugin.
      'calendar_block' => $base + array(
        'title' => t('Calendar block'),
        'help' => t('Display the view as a calendar block. Add a calendar view for a year, month, day, or week display and attach it to this block.'),
        'handler' => 'calendar_plugin_display_block',
        'theme' => 'views_view',
        'uses hook block' => TRUE,
        'use ajax' => TRUE,
        'use pager' => FALSE,
        'use more' => TRUE,
        'accept attachments' => TRUE,
        'admin' => t('Calendar block'),
        'help topic' => 'display-calendar',
      ),
      // Display plugin for calendar displays.
      'calendar_view' => $base + array(
        'title' => t('Calendar view'),
        'help' => t('This is a Year/Month/Day/Week view to attach to a calendar page or block. '),
        'handler' => 'calendar_plugin_display_attachment',
        'theme' => 'views_view',
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page'),
        'help topic' => 'display-calendar',
      ),
    ),
    'style' => array(
      // Style plugin for the navigation.
      'calendar' => $base + array(
        'title' => t('Calendar'),
        'help' => t('Creates back/next navigation and calendar links.'),
        'handler' => 'calendar_plugin_style',
        'theme' => 'calendar_main',
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'calendar', // Only used on calendar page or block displays.
        'even empty' => TRUE,
      ),
      // Style plugin for the attachments.
      'calendar_view' => $base + array(
        'title' => t('Calendar view'),
        'help' => t('Displays a calendar year, month, day, or week.'),
        'handler' => 'calendar_view_plugin_style',
        'theme' => 'calendar_month', // Templates are swapped in and out by the handler.
        'additional themes' => array(
          'calendar_year' => 'style',
          'calendar_month' => 'style',
          'calendar_mini' => 'style',
          'calendar_day' => 'style',
          'calendar_week' => 'style',
          ),
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
  );
}

/**
 * The plugin that handles a full calendar page.
 * 
 * The only style option that will be available is the calendar
 * style, which creates the navigation and links to other calendar
 * displays. All options for paging, row plugins, etc. are
 * deferred to the attachments.
 */
class calendar_plugin_display_page extends views_plugin_display_page {
  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    if (!$date_type = calendar_current_type($view)) {
      $date_type = $display->display_options['default_display'];
    }
    $view->date_type = $date_type;
    $view->default_display = $display->display_options['default_display'];
  }
  
  function get_style_type() { return 'calendar'; }
  
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('style_plugin', 'row_options', 'row_plugin', 'items_per_page'))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['displays'] = array();
    $display->display_options['style_plugin'] = 'calendar';
    $display->display_options['items_per_page'] = 0;
    $display->display_options['row_plugin'] = '';
    $display->display_options['defaults']['style_plugin'] = FALSE;
    $display->display_options['defaults']['style_options'] = FALSE;
    $display->display_options['defaults']['row_plugin'] = FALSE;
    $display->display_options['defaults']['row_options'] = FALSE;
    $display->display_options['defaults']['items_per_page'] = FALSE;
    $display->display_options['default_display'] = 'month';
  } 

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    switch ($form_state['section']) {
      case 'default_display':
        $form['#title'] .= t('Default display');
        $form['default_display'] = array(
          '#type' => 'radios',
          '#title' => t('Default display'),
          '#description' => t('The display that users will see if no arguments have been selected.'),
          '#default_value' => $this->get_option('default_display'),
          '#options' => calendar_display_types(),
        );
        break;
    }
  }

  /**
   * Store the option.
   */
  function options_submit($form, &$form_state) {
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case 'default_display':
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]);
        break;
    }
  }
    
  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    parent::options_summary($categories, $options);
    $display_types = calendar_display_types();    
    $options['default_display'] = array(
      'category' => 'page',
      'title' => t('Default display'),
      'value' => $display_types[$this->get_option('default_display')],
    );
  }
}

/**
 * The plugin that handles a calendar block.
 * 
 * The only style option that will be available is the calendar
 * style, which creates the navigation and links to other calendar
 * displays. All options for paging, row plugins, etc. are
 * deferred to the attachments.
 */
class calendar_plugin_display_block extends views_plugin_display_block {
  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    if (!$date_type = calendar_current_type($view)) {
      $date_type = $display->display_options['default_display'];
    }
    $view->date_type = $date_type;
    $view->default_display = $display->display_options['default_display'];
    if ($display_id == 'calendar_block') {
      $view->mini = TRUE;
      $view->block = TRUE;
    }
  }
  
  function get_style_type() { return 'calendar'; }
  
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('style_plugin', 'row_options', 'row_plugin', 'items_per_page'))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['displays'] = array();
    $display->display_options['style_plugin'] = 'calendar';
    $display->display_options['items_per_page'] = 0;
    $display->display_options['row_plugin'] = '';
    $display->display_options['defaults']['style_plugin'] = FALSE;
    $display->display_options['defaults']['style_options'] = FALSE;
    $display->display_options['defaults']['items_per_page'] = FALSE;
    $display->display_options['defaults']['row_plugin'] = FALSE;
    $display->display_options['defaults']['row_options'] = FALSE;
  } 
}

/**
 * The plugin that handles calendar attachment displays.
 * 
 * Adding year/month/day/week pages as attachments makes it
 * possible to use any style type, so they could be tables,
 * lists, teasers, or nodes as well as traditional calendar
 * pages.
 * 
 * Force 'inherit_arguments' to TRUE, and 'attachment_position' 
 * to 'after', and don't display those options in the UI.
 * 
 * Allows paging (regular attachments do not), and adds an option 
 * to choose what type of display this represents.
 */
class calendar_plugin_display_attachment extends views_plugin_display_attachment {

  function attach_to($display_id) {
    // See if we're attaching to a block rather than a page.
    if ($display_id == 'calendar_block') {
      $this->view->mini = TRUE;
      $this->view->block = TRUE;
    }
    parent::attach_to($display_id);
  }
  
  function date_types($type = 'month') {
    $types = calendar_display_types();
    return $types[$type];
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['inherit_argments'] = TRUE;
    $display->display_options['attachment_position'] = 'after';
    $display->display_options['date_type'] = 'month';
  }  

  /**
   * Provide the form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    switch ($form_state['section']) {
      case 'date_type':
        $form['#title'] .= t('Display type');
        $form['date_type'] = array(
          '#type' => 'radios',
          '#title' => t('Display'),
          '#options' => calendar_display_types(),
          '#description' => t('What type of display should this represent on the parent calendar to which it is attached?'),
          '#default_value' => $this->get_option('date_type'),
        );
        break;
    }
  }

  /**
   * Store the option.
   */
  function options_submit($form, &$form_state) {
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case 'date_type':
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]);
        break;
    }
  }
    
  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    parent::options_summary($categories, $options);

    $options['date_type'] = array(
      'category' => 'attachment',
      'title' => t('Calendar type'),
      'value' => $this->date_types($this->get_option('date_type')),
    );
    unset($options['attachment_position'], $options['inherit_arguments']);
  }
    
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('inherit_argments', 'attachment_position',))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }
  
  /**
   * Only render this attachment if it is the right one.
   * 
   * TODO Is there a way to not init this attachment at all, and if so,
   * will that save processing cycles?
   */
  function render() {
    if ($this->get_option('date_type') == $this->view->date_type) {
      return theme($this->theme_functions(), $this->view);
    } 
  }
}

/**
 * Style plugin to create the calendar navigation and links.
 * 
 * Used by the main calendar page and calendar block displays.
 */
class calendar_plugin_style extends views_plugin_style {
  
  /**
   * Set default options
   */
  function options(&$options) {
    $options['default_display'] = 'month';
  }

  function display_types($date_type = NULL, $option_type = 'names') {
    $ids = array();
    foreach (calendar_display_types() as $name => $type) {
      foreach ($this->view->display as $id => $display) {
        if ($display->display_plugin == 'calendar_view' 
        && $display->display_options['date_type'] == $name) {
          $ids[$name] = $id;
          $names[$name] = $display->display_title;
        }
      }
    }
    if ($date_type) {
      return $$option_type[$date_type];
    }
    return $$option_type;
  }
  
  /**
   * Calendar argument date fields used in this view.
   */
  function date_fields() {
    $date_fields = array();
    $calendar_fields = date_api_fields();
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
    if (!in_array('date_argument', array_keys($arguments))) {
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
    //$calendar_fields = array_keys(date_api_fields());
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
    if (!in_array('date_argument', array_keys($arguments))) {
      $form['error_markup'] = array(
        '#value' => t('You need to add a Calendar: Date argument before you can configure your calendar settings'),
        '#prefix' => '<div class="error form-item description">',
        '#suffix' => '</div>',
      );
      return;
    }

    $items = array();
    $date_fields = $this->date_fields();
    foreach ($date_fields as $field => $info) {
      $items[] = $info['name'];
    }
    $form['fields'] = array(
      '#value' => '<div class="form-item">'.
        '<strong>'. t('Date Fields') .'</strong><br />'. 
        theme('item_list', $items) .
        t('<div class="description">'. t('The date fields which will be used to filter the calendar. Change Date fields in the Calendar: Date argument.') .'</div>') .
        '</div>',
    );
    $view_displays = $this->display_types();
    $view_ids = $this->display_types(NULL, 'ids');
    $items = array();
    foreach (calendar_display_types() as $name => $value) {
      $title = isset($view_displays[$name]) ? $view_displays[$name] : t('None');
      $items[] = t('@date_type view: @name', array('@date_type' => $value, '@name' => $title));
    }
    $form['displays'] = array(
      '#value' => '<div class="form-item">'.
      '<strong>'. t('Displays') .'</strong><br />'. 
      theme('item_list', $items) .
      '<div class="description">'. t('The display that will be used for each type of calendar. Change these values by adding, removing, or editing the Calendar view attachments in this view.') .'</div>'.
      '</div>',
    );
    
    $form['default_display'] = array(
      '#title' => t('Default display'),
      '#description' => t('The display that users will see if no arguments have been selected.'),
      '#default_value' => $this->options['default_display'],
      '#type' => 'select',
      '#options' => calendar_display_types(),
    );
  }
  
  function options_submit($form, &$form_state) {
    $form_state['values']['style_options']['date_fields'] = array_keys($form_state['values']['style_options']['default_display']);
  }

  function query() {
    
    include_once(drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
        
    $style_options = $this->view->style_plugin->options;
    
    // Evaluate our argument values and figure out which 
    // calendar display we need to create.
    $i = 0;    
    foreach ($this->view->argument as $delta => $argument) {
      if ($argument['id'] == 'date_argument') {
        $this->view->date_type = $argument['handler']->date_type;
        $this->view->date_arg = $argument['handler']->argument;
        $this->view->date_arg_pos = $i;
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
    $style_options = $this->view->style_plugin->options;
    $this->view->calendar_display = $style_options[$this->view->date_type];
    
    // Create an array of all the valid calendar types for this view.
    $this->view->date_types = $this->display_types();
            
    // bring the node type into the query so we can use it in the theme
    $this->view->query->add_field('node', 'type');
      
    parent::query();
  }
  
  /**
   * Render the calendar navigation style.
   */
  function render() {
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}

/**
 * Style plugin to render the year, month, week, or day calendar view.
 */
class calendar_view_plugin_style extends calendar_plugin_style {
  
  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    $view->mini_name_size = $this->view->style_plugin->options['mini_name_size'];
    $view->full_name_size = $this->view->style_plugin->options['full_name_size'];
  }
  
  /**
   * Set default options
   */
  function options(&$options) {
    $options['mini_name_size'] = 1;
    $options['full_name_size'] = 3;
  }

  /**
   * Style options.
   */
  function options_form(&$form, &$form_state) {
    $form['mini_name_size'] = array(
      '#title' => t('Mini calendar day of week names'),
      '#default_value' => $this->options['mini_name_size'],
      '#type' => 'radios',
      '#options' => array(1 => t('First letter of name'), 2 => t('First two letters of name'), 3 => t('Abbreviated name'), 99 => t('Full name')),
      '#description' => t('The way day of week names should be displayed in a mini calendar.'),
      );
    $form['full_name_size'] = array(
      '#title' => t('Large calendar day of week names'),
      '#default_value' => $this->options['full_name_size'],
      '#type' => 'radios',
      '#options' => array(1 => t('First letter of name'), 2 => t('First two letters of name'), 3 => t('Abbreviated name'), 99 => t('Full name')),
      '#description' => t('The way day of week names should be displayed in a full sized calendar.'),
      );
  }
 
  /**
   * Render the calendar attachment style.
   */
  function render() {
    // Adjust the theme to match the currently selected calendar type.
    $this->definition['theme'] = 'calendar_'. $this->view->date_type;
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}