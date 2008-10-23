<?php
// $Id$
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
   * Display validation.
   */
  function validate() {
    $errors = parent::validate();
    
    $arguments = $this->display->handler->get_option('arguments');
    if (!in_array('date_argument', array_keys($arguments))) {
      $errors[] = t("The Calendar display '@display_title' will not work without a Date argument.", array('@display_title' => $this->definition['title']));      
    }
    elseif ($arguments['date_argument']['default_action'] != 'default' || $arguments['date_argument']['default_argument_type'] != 'date') {
      $errors[] = t("The Date argument @title in this view must be set up to provide a default value set to the current date.", array('@title' => $arguments['date_argument']['title']));      
    }
    
    return $errors;
  }

  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    $view->calendar_popup = $this->get_option('calendar_popup');
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
    $options['calendar_popup'] = 0;
    $options['style_plugin'] = 'calendar_nav';
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
      'title' => theme('advanced_help_topic', 'calendar', 'settings') . t('Legend colors'),
      'value' => t('Edit'),
    );
    $popup_options = $this->popup_options();
    $options['calendar_popup'] = array(
      'category' => 'calendar_settings',
      'title' => theme('advanced_help_topic', 'calendar', 'settings') . t('Date changer'),
      'value' => $popup_options[$this->get_option('calendar_popup')],
    );
  }
  
  function popup_options() {
    return array(0 => t('No'), 1 => t('Yes'));
  }
   
  function options_form(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_form($form, $form_state);
       
    switch ($form_state['section']) {
      case 'calendar_popup':
        $form['#title'] .= t('Date changer');
        $form['calendar_popup'] = array(
          '#type' => 'radios',
          '#default_value' => $this->get_option('calendar_popup'),
          '#options' => $this->popup_options(),
          '#description' => t('Display a popup calendar date selector?'),
          );        
        break;
      
      case 'calendar_colors':
        $method =  'types';
        // TODO Move the embedded styles other than the color into a stylesheet.
        $form['#title'] .= t('Legend colors');
        $form['calendar_colors']['#tree'] = TRUE;
        $form['calendar_colors']['#prefix'] = t('<div class="form-item"><label>Content Type</label><p>Set a hex color value (like #ffffff) to use in the calendar legend for each content type. Types with empty values will have no stripe in the calendar and will not be added to the legend.</p></div>');
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
      case 'calendar_popup':
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]['popup']);
        break;
      case 'calendar_colors':
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]['color']);
        break;
    }
  }  

}

