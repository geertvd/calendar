<?php
/**
 * @todo
 */
class calendar_plugin_display_extender extends views_plugin_display_extender {
  function __construct(&$view, &$display) {
    if ($display->definition['handler'] == 'views_plugin_display_page') {
      $this->view = $view;
      $this->display = $display;
    }
    else {
      return FALSE;
    }
  }


  /**
   * Provide a form to edit options for this plugin.
   */
  function options_definition(&$options) { 
    $options = parent::option_definition();
    $options['calendar_colors'] = array('default' => array());
    $options['calendar_colors_vocabulary'] = array('default' => array());
    $options['calendar_colors_taxonomy'] = array('default' => array());
    $options['calendar_colors_group'] = array('default' => array());
    $options['calendar_popup'] = array('default' => 0);
    $options['calendar_date_link'] = array('default' => '');
    return $options;
  }

  /**
   * Provide a form to edit options for this plugin.
   */
  function options_form(&$form, &$form_state) { 
    // It is very important to call the parent function here:
    parent::options_form($form, $form_state);

    switch ($form_state['section']) {
      case 'calendar_popup':
        $form['#title'] .= t('Date changer');
        $form['calendar_popup'] = array(
          '#type' => 'radios',
          '#default_value' => $this->options['calendar_popup'],
          '#options' => $this->popup_options(),
          '#description' => t('Display a popup calendar date selector?'),
          );        
        break;

      case 'calendar_date_link':
        $form['#title'] .= t('Add new date link');
        $form['calendar_date_link'] = array(
          '#type' => 'radios',
          '#default_value' => $this->options['calendar_date_link'],
          '#options' => array('' => t('No link')) + node_type_get_names(),
          '#description' => t('Display a link to add a new date of the specified content type. Displayed only to users with appropriate permissions.'),
          );        
        break;

      case 'calendar_colors':
        $method =  'types';
        // TODO Move the embedded styles other than the color into a stylesheet.
        $form['#title'] .= t('Content Type Legend Colors');
        $form['calendar_colors']['#tree'] = TRUE;
        $form['calendar_colors']['#prefix'] = '<div class="form-item"><label>' . t('Content Type') . '</label><p>' . t('Set a hex color value (like #ffffff) to use in the calendar legend for each content type. Types with empty values will have no stripe in the calendar and will not be added to the legend.') . '</p></div>';

        $colors = $this->options['calendar_colors'];   

        switch ($method) {
          case 'types':
            $color_types = node_type_get_names();
            break;
        }
        foreach ($color_types as $key => $name) {
          $form['calendar_colors']['color'][$key] = array(
            '#title' => $name,
            '#type' => 'textfield',
            '#default_value' => isset($colors[$key]) ? $colors[$key] : '#ffffff',
            '#size' => 7,
            '#maxlength' => 7,
          );
        }
        break;

      case 'calendar_colors_vocabulary':
        $taxonomies = taxonomy_get_vocabularies();
        $options = array();
        foreach ($taxonomies as $vid => $vocab) {
          $options[$vid] = $vocab->name;
        }
        $colors_vocabulary = $this->options['calendar_colors_vocabulary'];
        $form['#title'] .= t('Vocabulary Legend Types');
        $form['calendar_colors_vocabulary'] = array(
          '#type' => 'checkboxes',
          '#default_value' => isset($colors_vocabulary) ? $colors_vocabulary : array(),
          '#multiple' => TRUE,
          '#options' => $options,
          );
        $form['calendar_colors_vocabulary']['#prefix'] = '<div class="form-item"><label>' . t('Vocabularies') . '</label>' . t('Select vocabularies to use for setting calendar legend colors by taxonomy term. This works best for vocabularies with only a limited number of possible terms.') .'</div>';
        break;

      case 'calendar_colors_taxonomy':
        $taxonomies = (array) $this->options['calendar_colors_vocabulary'];
        $colors_taxonomy = $this->options['calendar_colors_taxonomy'];
        $form['#title'] .= t('Taxonomy Legend Colors');
        $form['calendar_colors_taxonomy']['#prefix'] = '';
        if (empty($taxonomies)) {
          $form['calendar_colors_taxonomy']['#prefix'] .= '<div class="form-item warning">' . t('Please select Legend vocabularies first!') .'</div>';
        }
        $form['calendar_colors_taxonomy']['#prefix'] .= '<div class="form-item"><label>' . t('Taxonomy Terms') . '</label><p>' . t('Set a hex color value (like #ffffff) to use in the calendar legend for each taxonomy term. Terms with empty values will have no stripe in the calendar and will not be added to the legend. <strong>IMPORTANT!</strong>You must add the taxonomy field, Taxonomy: Term ID, to the view for this to do anything. You can exclude it from output if you do not want to see it in the view, but the field must be included.') . '</p></div>';
        $form['calendar_colors_taxonomy']['#tree'] = TRUE;

        $colors = $this->options['calendar_colors']; 

        foreach ($taxonomies as $vid => $taxonomy) {
          $vocab = taxonomy_get_tree($vid);
          foreach ($vocab as $tid => $term) {
            $form['calendar_colors_taxonomy']['color'][$term->tid] = array(
              '#title' => t($term->name),
              '#type' => 'textfield',
              '#default_value' => isset($colors_taxonomy[$term->tid]) ? $colors_taxonomy[$term->tid] : '#ffffff',
              '#size' => 7,
              '#maxlength' => 7,
            );
          }
        }
        break;

      case 'calendar_colors_group':
        $colors_group = $this->option['calendar_colors_group'];
        $form['#title'] .= t('Group Legend Colors');
        $form['calendar_colors_group']['#prefix'] = '';
        $form['calendar_colors_group']['#prefix'] .= '<div class="form-item"><label>' . t('Group') . '</label><p>' . t('Set a hex color value (like #ffffff) to use in the calendar legend for each group. Groups with empty values will have no stripe in the calendar and will not be added to the legend.') .'</p></div>';
        $form['calendar_colors_group']['#tree'] = TRUE;
        $form['calendar_colors_group']['colorpicker'] = array(
          '#type' => 'calendar_colorpicker',
          '#prefix' => '<div class="clear-block"><div style="float:left">',
          '#suffix' => '</div>',
        );
        $groups = og_all_groups_options();
        foreach ($groups as $gid => $group_name){
          $form['calendar_colors_group']['color'][$gid] = array(
	          '#title' => t($group_name),
	          '#type' => 'calendar_colorfield',
	          '#default_value' => isset($colors_group[$gid]) ? $colors_group[$gid] : '#ffffff',
	          '#calendar_colorpicker' => 'calendar-colors-group-colorpicker',
	          '#prefix' => '<div style="float:left;margin-right:10px">',
	          '#suffix' => '</div>',
	          );
	      }
	      $form['calendar_colors_group']['color']['#suffix'] = '</div>';
        break;
    }
  }

  function popup_options() {
    return array(0 => t('No'), 1 => t('Yes'));
  }

  /**
   * Validate the options form.
   */
  function options_validate(&$form, &$form_state) { 

  }

  /**
   * Handle any special handling on the validate form.
   */
  function options_submit(&$form, &$form_state) { 
    // It is very important to call the parent function here:
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case 'calendar_popup':
        $this->options[$form_state['section']] = $form_state['values'][$form_state['section']]['popup'];
        break;
      case 'calendar_colors':
      case 'calendar_colors_taxonomy':
      case 'calendar_colors_group':
        $this->options[$form_state['section']] = $form_state['values'][$form_state['section']]['color'];
        break;
      case 'calendar_colors_vocabulary':
        $this->options[$form_state['section']] = array_filter($form_state['values'][$form_state['section']]);
        break;
      case 'calendar_date_link':
        $this->options[$form_state['section']] = ($form_state['values'][$form_state['section']]);
        variable_set('calendar_date_link_'. $form_state['values']['calendar_date_link'], $this->display->handler->options['path']);
        break;
    }

  }

  /**
   * Set up any variables on the view prior to execution.
   */
  function pre_execute() { }

  /**
   * Inject anything into the query that the display_extender handler needs.
   */
  function query() { }

  /**
   * Provide the default summary for options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) { 

    // It is very important to call the parent function here:
    parent::options_summary($categories, $options);

    $variables = array(
      'module' => 'calendar', 
      'topic' => 'settings',
    );
    $categories['calendar_settings'] = array(
      'title' => theme('advanced_help_topic', $variables) . t('Calendar settings'),
      'column' => 'second',
    );

    $colors = $this->options['calendar_colors'];
    $colors_taxonomy = $this->options['calendar_colors_taxonomy'];
    $options['calendar_colors'] = array(
      'category' => 'calendar_settings',
      'title' => t('Legend Content Types'),
      'value' => t('Edit'),
    );
    $options['calendar_colors_vocabulary'] = array(
      'category' => 'calendar_settings',
      'title' => t('Legend Vocabularies'),
      'value' => t('Edit'),
    );
    $options['calendar_colors_taxonomy'] = array(
      'category' => 'calendar_settings',
      'title' => t('Legend Terms'),
      'value' => t('Edit'),
    );
    if (function_exists('og_all_groups_options')) {
      $colors_group = $this->options['calendar_colors_group'];
      $options['calendar_colors_group'] = array(
        'category' => 'calendar_settings',
        'title' => t('Legend Groups'),
        'value' => t('Edit'),
      );
    }
    $popup_options = $this->popup_options();
    $default = $this->options['calendar_popup'];
    $options['calendar_popup'] = array(
      'category' => 'calendar_settings',
      'title' => t('Date changer'),
      'value' => isset($default) ? $popup_options[$default] : $popup_options[0],
    );
    $default = $this->options['calendar_date_link'];
    $options['calendar_date_link'] = array(
      'category' => 'calendar_settings',
      'title' => t('Add new date link'),
      'value' => !empty($default) ? check_plain(node_type_get_name($default)) : t('None'),
    );

  }

  /**
   * Static member function to list which sections are defaultable
   * and what items each section contains.
   */
  function defaultable_sections(&$sections, $section = NULL) { }
}