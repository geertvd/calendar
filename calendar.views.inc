<?php
//$Id$
/**
 * @file
 * 
 * Creates calendar displays of Views results.
 * 
 * Create a new calendar by enabling or cloning the default calendar,
 * changing the date argument to use the correct date field(s), and setting
 * up the year, month, day, week, and block views with the desired styles 
 * and fields.
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
 * styles are desired for those pages. 
 * 
 * Calendar views are attachments to create the year, month, day,
 * and week displays. They can be set to use any style, either a
 * calendar style or any other Views style, like teasers or lists.
 * If you don't want to use one of them, don't attach it to
 * anything. Only the attached views will show up in the calendar.
 * 
 * A calendar block will create a calendar block for the
 * view results. Attach a block view to the block and set up the
 * desired style in the block view. 
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
  $data = array(
    'module' => 'calendar', // This just tells our themes are elsewhere.
    'display' => array(
      // Main calendar display plugin.
      'calendar' => $base + array(
        'title' => t('Calendar page'),
        'help' => t('Calendar page display. Attach one or more of the Year/Month/Day/Week views to this page.'),
        'handler' => 'calendar_plugin_display_page',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'uses hook menu' => TRUE,
        'use ajax' => TRUE,
        'use pager' => FALSE,
        'accept attachments' => TRUE,
        'admin' => t('Calendar page'),
        'help topic' => 'display-calendar',
      ),
      // Calendar block display plugin.
      'calendar_block' => $base + array(
        'title' => t('Calendar block'),
        'help' => t('Calendar block display. Attach a block view to this block'),
        'handler' => 'calendar_plugin_display_block',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'uses hook block' => TRUE,
        'use ajax' => TRUE,
        'use pager' => FALSE,
        'use more' => TRUE,
        'accept attachments' => TRUE,
        'admin' => t('Calendar block'),
        'help topic' => 'display-calendar',
      ),
      // Display plugins for calendar displays.
      'calendar_year' => $base + array(
        'title' => t('Year view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_year',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page year'),
        'help topic' => 'display-calendar',
      ),
      'calendar_month' => $base + array(
        'title' => t('Month view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_month',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page month'),
        'help topic' => 'display-calendar',
      ),
      'calendar_day' => $base + array(
        'title' => t('Day view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_day',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page day'),
        'help topic' => 'display-calendar',
      ),
      'calendar_week' => $base + array(
        'title' => t('Week view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_week',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page week'),
        'help topic' => 'display-calendar',
      ),
      'calendar_block_view' => $base + array(
        'title' => t('Block view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_block',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar block view'),
        'help topic' => 'display-calendar',
      ),
    ),
    'style' => array(
      // Style plugin for the navigation.
      'calendar_nav' => $base + array(
        'title' => t('Calendar navigation'),
        'help' => t('Creates back/next navigation and calendar links.'),
        'handler' => 'calendar_plugin_style',
        'theme' => 'calendar_main',
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => FALSE,
        'type' => 'calendar', // Only used on calendar page or block displays.
        'even empty' => TRUE,
      ),
      'calendar_style' => array(
        'title' => t('Calendar'),
        'help' => t('Displays Views results in a calendar.'),
        'handler' => 'calendar_view_plugin_style',
        'theme' => 'calendar_month',
        'file' => 'theme.inc',
        'path' => "$path/theme",
        'additional themes' => array(
          'calendar_year' => 'style',
          'calendar_day' => 'style',
          'calendar_week' => 'style',
          'calendar_mini' => 'style',
          ),
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
  );
  return $data;
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
    $display->display_options['style_plugin'] = 'calendar_nav';
    $display->display_options['items_per_page'] = 0;
    $display->display_options['row_plugin'] = '';
    $display->display_options['defaults']['style_plugin'] = FALSE;
    $display->display_options['defaults']['style_options'] = FALSE;
    $display->display_options['defaults']['row_plugin'] = FALSE;
    $display->display_options['defaults']['row_options'] = FALSE;
    $display->display_options['defaults']['items_per_page'] = FALSE;
  } 
  
  function option_definition () {
    $options = parent::option_definition();

    $options['calendar_colors'] = array();
    return $options;
  }

  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    // It is very important to call the parent function here:
    parent::options_summary($categories, $options);

    $categories['calendar_settings'] = array(
      'title' => t('Calendar settings'),
    );
    
    $colors = $this->get_option('calendar_colors');
    $options['calendar_colors'] = array(
      'category' => 'calendar_settings',
      'title' => t('Legend colors'),
      'value' => t('Edit'),
    );
  }
   
  function options_form(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_form($form, $form_state);
       
    switch ($form_state['section']) {
      case 'calendar_colors':
        //TODO Add in methods other than content types for setting legend colors.
        $method =  'types';
  
        // TODO Move the embedded styles other than the color into a stylesheet.
       $form['#title'] .= t('Legend colors');
       $form['#prefix'] = t('<div class="form-item"><label>Content Type</label><p>Set a hex color value (like #ffffff) to use in the calendar legend for each content type. Types with empty values will have no stripe in the calendar and will not be added to the legend.</p></div>');
       $form['calendar_colors']['#tree'] = TRUE;
       $form['calendar_colors']['colorpicker'] = array(
          '#type' => 'calendar_colorpicker',
          '#prefix' => '<div class="clear-block"><div style="float:left">',
          '#suffix' => '</div>',
        );
        $colors = $this->get_option('calendar_colors'); 
  
        switch ($method) {
          case 'types':
            $color_types = node_get_types('names');
            break;
        }
        foreach ($color_types as $key => $name) {
          $form['calendar_colors']['color'][$key] = array(
            '#title' => $name,
            '#type' => 'calendar_colorfield',
            '#default_value' => isset($colors[$key]) ? $colors[$key] : '#ffffff',
            '#calendar_colorpicker' => 'calendar-colors-colorpicker',
            '#prefix' => '<div style="float:left;margin-right:10px">',
            '#suffix' => '</div>',
          );
        }
        $form['calendar_colors']['color']['#suffix'] = '</div>';
        break;
    }
  }

 /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit($form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case 'calendar_colors':
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]['color']);
        break;
    }
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
    if (!isset($display->display_options['style_options'])) {
      return;
    }
    if (!$granularity = calendar_current_type($view)) {
      $granularity = $display->display_options['style_options']['default_display'];
    }
    $view->granularity = $granularity;
    $view->default_display = $display->display_options['style_options']['default_display'];
    if ($display->id == 'calendar_block') {
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
    $display->display_options['style_plugin'] = 'calendar_nav';
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
    if (substr($display_id, 0, 14) == 'calendar_block') {
      $this->view->mini = TRUE;
      $this->view->block = TRUE;
      $this->view->parent_id = $display_id;
    }
    elseif (substr($display_id, 0, 8) == 'calendar') {
      $this->view->parent_id = $display_id;
      $this->view->calendar_colors = $this->view->display[$display_id]->handler->options['calendar_colors'];
    }
    parent::attach_to($display_id);
    
  }
  
  function display_types($type = 'month') {
    $types = calendar_display_types();
    return $types[$type];
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['inherit_argments'] = TRUE;
    $display->display_options['attachment_position'] = 'after';
    $display->display_options['calendar_type'] = 'month';
  }  

  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    parent::options_summary($categories, $options);
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
   */
  function render() {
    if (!empty($this->view->block) || $this->definition['handler'] == 'calendar_plugin_'. $this->view->granularity) {
      return theme($this->theme_functions(), $this->view);
    } 
  }
}

class calendar_plugin_year extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'year';
  }  
}

class calendar_plugin_month extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'month';
  }  
}

class calendar_plugin_day extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'day';
  }  
}

class calendar_plugin_week extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'week';
  }  
}

class calendar_plugin_block extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'month';
  }  
}

/**
 * Style plugin to create the calendar navigation and links.
 * 
 * Used by the main calendar page and calendar block displays.
 */
class calendar_plugin_style extends views_plugin_style {
  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    $view->display_types = $this->display_types();
  }
    
  function display_types($granularity = NULL, $option_type = 'names') {
    $ids = array();
    $names = array();
    foreach (calendar_display_types() as $name => $type) {
      foreach ($this->view->display as $id => $display) {
        if (in_array($display->display_plugin, array('calendar_year', 'calendar_month', 'calendar_day', 'calendar_week'))) {
          if ($display->display_plugin == 'calendar_'. $name) {
            $attachments = array_filter($display->display_options['displays']);
            if (isset($attachments['calendar'])) {
              $ids[$name] = $id;
              $names[$name] = $display->display_title;
            }
          }
        }
      }
    }
    if ($granularity) {
      return $$option_type[$granularity];
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
          $field = $calendar_fields['name'][$date_field];
          $handler = views_get_handler($field['table_name'], $field['field_name'], 'field');
          if ($handler) {
            $date_fields[$date_field] = $field;
            $date_fields[$date_field]['name'] = $handler->ui_name();
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
      $errors[] = t('Style @style will not work without the Date argument.', array('@style' => $this->definition['title']));      
    }
    elseif ($arguments['date_argument']['default_action'] != 'default' || $arguments['date_argument']['default_argument_type'] != 'date') {
      $errors[] = t('The Date argument must be set up to provide a default value set to the current date.');      
    }
      
    
    // TODO fix the following validation code to work correctly in Views2.
  
    // CCK date fields cannot use grouped handler.
    //$calendar_fields = array_keys(date_api_fields());
    //foreach ($view['field'] as $delta => $field) {
    //  if (in_array($field['field_name'], $calendar_fields) && $field['id'] == 'content_views_field_handler_group') {
        //form_error($form['field_name'][$delta], t('Calendar CCK Date fields must be set to \'Do not group multiple values\'.'));
    //  }
    //}
    return $errors;
  }

  function query() {
    
    include_once(drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
        
    $style_options = $this->view->style_plugin->options;
    
    // Evaluate our argument values and figure out which 
    // calendar display we need to create.
    $i = 0;   
    foreach ($this->view->argument as $id => $argument) {
      if ($argument->field == 'date_argument') {
        // TODO Decide if we want to provide a date here or not.
        // Adding this now is to prevent fatal errors later if the
        // view is used in unexpected ways without a date being set.
        if (empty($argument->min_date)) {
          $value = $argument->get_default_argument();
          $range = $argument->date_handler->arg_range($value);
          $argument->min_date = $range[0];
          $argument->max_date = $range[1];
        }
        $this->view->granularity = !empty($argument->granularity) ? $argument->granularity : $argument->options['granularity'];
        $this->view->date_arg = !empty($this->view->args) ? $this->view->args[$argument->position] : '';
        $this->view->date_arg_pos = $i;
        $this->view->year = isset($argument->year) ? $argument->year : NULL;
        $this->view->month = isset($argument->month) ? $argument->month: NULL;
        $this->view->day = isset($argument->day) ? $argument->day : NULL;
        $this->view->week = isset($argument->week) ? $argument->week : NULL;
        $this->view->min_date = $argument->min_date;
        $this->view->max_date = $argument->max_date;
        $this->view->min_date_date = date_format($this->view->min_date, DATE_FORMAT_DATE);
        $this->view->max_date_date = date_format($this->view->max_date, DATE_FORMAT_DATE);
      }
      $i++;
    }
    $this->view->display_types = $this->display_types();
    $keys = drupal_map_assoc(array_keys(calendar_display_types()));
    $this->view->calendar_display = $keys[$this->view->granularity];
           
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
    $view->name_size = $this->options['name_size'];
    $view->with_weekno = $this->options['with_weekno'];
  }
  
  /**
   * Set default options
   */
  function options(&$options) {
    $options['name_size'] = 3;
    $options['display_type'] = 'month';
    $options['with_weekno'] = 0;
  }

  /**
   * Style options.
   */
  function options_form(&$form, &$form_state) {
    $form['display_type'] = array(
      '#type' => 'radios',
      '#title' => t('Display'),
      '#options' => calendar_display_types(),
      '#description' => t('What type of calendar template should be used for this style?'),
      '#default_value' => $this->options['display_type'],
    );
    $form['name_size'] = array(
      '#title' => t('Calendar day of week names'),
      '#default_value' => $this->options['name_size'],
      '#type' => 'radios',
      '#options' => array(1 => t('First letter of name'), 2 => t('First two letters of name'), 3 => t('Abbreviated name'), 99 => t('Full name')),
      '#description' => t('The way day of week names should be displayed in a calendar.'),
      );
    $form['with_weekno'] = array(
      '#title' => t('Show week numbers'),
      '#default_value' => $this->options['with_weekno'],
      '#type' => 'radios',
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#description' => t('Whether or not to show week numbers in the left column of calendar weeks and months.'),
      );

  }
  
   /**
   * Render the calendar attachment style.
   */
  function render() {
    // Adjust the theme to match the currently selected calendar type.
    $this->definition['theme'] = 'calendar_'. $this->options['display_type'];
    $this->view->hide_admin_links = TRUE;
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}