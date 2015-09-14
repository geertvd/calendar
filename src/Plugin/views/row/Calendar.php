<?php

/**
 * @file
 * Contains \Drupal\calendar\Plugin\views\row\Calendar.
 */

namespace Drupal\calendar\Plugin\views\row;

use Drupal\Core\Entity\Entity;
use Drupal\Core\Form\FormStateInterface;
use Drupal\views\Plugin\views\display\DisplayPluginBase;
use Drupal\views\Plugin\views\row\RowPluginBase;
use Drupal\views\ViewExecutable;

/**
 * Plugin which creates a view on the resulting object and formats it as a
 * Calendar entity.
 *
 * @ViewsRow(
 *   id = "calendar_row",
 *   title = @Translation("Calendar entities"),
 *   help = @Translation("Display the content as calendar entities."),
 *   theme = "views_view_row_calendar",
 *   register_theme = FALSE,
 * )
 */
class Calendar extends RowPluginBase {

  // Stores the entities loaded with pre_render.
  // @TODO redefine
  var $entities = array();

  /**
   * {@inheritdoc}
   */
  public function init(ViewExecutable $view, DisplayPluginBase $display, array &$options = NULL) {
    parent::init($view, $display, $options);

    // TODO needed?
//     $this->base_table = $view->base_table;
    // $this->base_field = $view->base_field;
  }

  /**
   * {@inheritdoc}
   */
  protected function defineOptions() {
    $options = parent::defineOptions();
    $options['date_fields'] = ['default' => []];
    $options['calendar_date_link'] = ['default' => ''];
    $options['colors'] = [
      'contains' => [
        'legend' => ['default' => ''],
        'calendar_colors_type' => ['default' => array()],
        'taxonomy_field' => ['default' => ''],
        'calendar_colors_vocabulary' => ['default' => array()],
        'calendar_colors_taxonomy' => ['default' => array()],
        'calendar_colors_group' => ['default' => array()],
      ]];
    return $options;
  }

  /**
   * {@inheritdoc}
   */
  public function buildOptionsForm(&$form, FormStateInterface $form_state) {
    parent::buildOptionsForm($form, $form_state);

    $form = [];

    $form['markup'] = [
      '#markup' => $this->t("The calendar row plugin will format view results as calendar items. Make sure this display has a 'Calendar' format and uses a 'Date' contextual filter, or this plugin will not work correctly."),
    ];

    $form['calendar_date_link'] = [
      '#title' => t('Add new date link'),
      '#type' => 'select',
      '#default_value' => $this->options['calendar_date_link'],
      '#options' => [
        '' => $this->t('No link'),
      ] + node_type_get_names(),
      '#description' => $this->t('Display a link to add a new date of the specified content type. Displayed only to users with appropriate permissions.'),
    ];

    $form['colors'] = [
      '#type' => 'fieldset',
      '#title' => $this->t('Legend Colors'),
      '#description' =>  $this->t('Set a hex color value (like #ffffff) to use in the calendar legend for each content type. Items with empty values will have no stripe in the calendar and will not be added to the legend.'),
    ];

    $options = [];
    if ($this->view->getBaseTables()['node_field_data']) {
      $options['type'] = $this->t('Based on Content Type');
    }
    if (\Drupal::moduleHandler()->moduleExists('taxonomy')) {
      $options['taxonomy'] = $this->t('Based on Taxonomy');
    }

    // If no option is available, stop here.
    if (empty($options)) {
      return;
    }

    $form['colors']['legend'] = [
      '#title' => $this->t('Stripes'),
      '#description' => $this->t('Add stripes to calendar items.'),
      '#type' => 'select',
      '#options' => $options,
      '#empty_value' => $this->t('None'),
      '#default_value' => $this->options['colors']['legend'],
    ];

    if ($this->view->getBaseTables()['node_field_data']) {
      $colors = $this->options['colors']['calendar_colors_type'];
      $type_names = node_type_get_names();
      foreach ($type_names as $key => $name) {
        $form['colors']['calendar_colors_type'][$key] = [
          '#title' => $name,
          '#default_value' => isset($colors[$key]) ? $colors[$key] : CALENDAR_EMPTY_STRIPE,
          '#dependency' => ['edit-row-options-colors-legend' => ['type']],
          '#type' => 'textfield',
          '#size' => 7,
          '#maxlength' => 7,
          '#element_validate' => ['calendar_validate_hex_color'],
          '#prefix' => '<div class="calendar-colorpicker-wrapper">',
          '#suffix' => '<div class="calendar-colorpicker"></div></div>',
          '#attributes' => ['class' => ['edit-calendar-colorpicker']],
          '#attached' => [
            // Add Farbtastic color picker and the js to trigger it.
            // TODO make the color picker actually work
            'library' => [
              ['system', 'farbtastic', 'calendar/calendar-colorpicker'],
            ],
          ],
        ];
      }
    }

    if (\Drupal::moduleHandler()->moduleExists('taxonomy')) {
      // Get the display's field names of taxonomy fields.
      $vocabulary_field_options = [];
      $fields = $this->displayHandler->getOption('fields');
      foreach ($fields as $name => $field_info) {
        // Select the proper field type.
        if (!empty($field_info['type']) && $field_info['type'] == 'entity_reference_label') {
          $vocabulary_field_options[$name] = $field_info['label'] ?: $name;
        }
      }
      $form['colors']['taxonomy_field'] = [
        '#title' => t('Term field'),
        '#type' => !empty($vocabulary_field_options) ? 'select' : 'hidden',
        '#default_value' => $this->options['colors']['taxonomy_field'],
        '#empty_value' => $this->t('None'),
        '#description' => $this->t("Select the taxonomy term field to use when setting stripe colors. This works best for vocabularies with only a limited number of possible terms."),
        '#options' => $vocabulary_field_options,
        '#dependency' => ['edit-row-options-colors-legend' => ['taxonomy']],
      ];

      if (empty($vocabulary_field_options)) {
        $form['colors']['taxonomy_field']['#options'] = ['' => ''];
        $form['colors']['taxonomy_field']['#suffix'] = $this->t('You must add a term field to this view to use taxonomy stripe values. This works best for vocabularies with only a limited number of possible terms.');
      }

      // Get the Vocabulary names.
      $vocab_vids = array();
      foreach ($vocabulary_field_options as $field_name => $label) {
        $field_config = \Drupal::entityManager()->getStorage('field_config')->loadByProperties(['field_name' => $field_name]);

        // @TODO refactor
        reset($field_config);
        $key = key($field_config);

        $data = \Drupal::config('field.field.' . $field_config[$key]->getOriginalId())->getRawData();

        $target_bundles = $data['settings']['handler_settings']['target_bundles'];
        reset($target_bundles);
        $vocab_vids[$field_name] = key($target_bundles);

      }

      $this->options['colors']['calendar_colors_vocabulary'] = $vocab_vids;

      $form['colors']['calendar_colors_vocabulary'] = array(
        '#title' => t('Vocabulary Legend Types'),
        '#type' => 'value',
        '#value' => $vocab_vids,
      );

      // Get the Vocabulary term id's and map to colors.
      $term_colors = $this->options['colors']['calendar_colors_taxonomy'];
      foreach ($vocab_vids as $field_name => $vid) {
        $vocab = \Drupal::entityManager()->getStorage("taxonomy_term")->loadTree($vid);
        foreach ($vocab as $key => $term) {
          $form['colors']['calendar_colors_taxonomy'][$term->tid] = [
            '#title' => $this->t($term->name),
            '#default_value' => isset($term_colors[$term->tid]) ? $term_colors[$term->tid] : CALENDAR_EMPTY_STRIPE,
            '#access' => !empty($vocabulary_field_options),
            '#dependency_count' => 2,
            '#dependency' => [
              'edit-row-options-colors-legend' => ['taxonomy'],
              'edit-row-options-colors-taxonomy-field' => [$field_name],
            ],
            '#type' => 'textfield',
            '#size' => 7,
            '#maxlength' => 7,
            '#element_validate' => [ 'calendar_validate_hex_color' ],
            '#prefix' => '<div class="calendar-colorpicker-wrapper">',
            '#suffix' => '<div class="calendar-colorpicker"></div></div>',
            '#attributes' => ['class' => ['edit-calendar-colorpicker']],
            '#attached' => [
              // Add Farbtastic color picker.
              // @TODO make this actually work
              'library' => [
                ['system', 'farbtastic', 'calendar/calendar-colorpicker'],
              ],
            ],
          ];
        }
      }
    }
  }

  function options_submit(&$form, &$form_state) {
    parent::options_submit($form, $form_state);
    if ($this->view->base_table == 'node') {
      if (isset($this->view->display['default']->display_options['link_display'])) {
        $default_display = $this->view->display['default']->display_options['link_display'];
        $path = $this->view->display[$default_display]->handler->get_option('path');
        // If this display has been set up as a default tab, the current path
        // is actually the base path, i.e. if the path is 'calendar/month'
        // and this is a default tab, the path for this display will actually
        // be 'calendar'.
        if ($this->view->display[$default_display]->handler->options['menu']['type'] == 'default tab') {
          $parts = explode('/', $path);
          array_pop($parts);
          $path = implode('/', $parts);
        }
        calendar_clear_link_path($path);
        if (!empty($form_state['values']['row_options']['calendar_date_link'])) {
          $node_type = $form_state['values']['row_options']['calendar_date_link'];
          calendar_set_link('node', $node_type, $path);
        }
      }
    }
  }

  function pre_render($values) {

    // Preload each entity used in this view from the cache.
    // Provides all the entity values relatively cheaply, and we don't
    // need to do it repeatedly for the same entity if there are
    // multiple results for one entity.
    $ids = array();
    foreach ($values as $row) {
      // Use the $id as the key so we don't create more than one value per entity.
      // This alias will automatically adjust to be the id of related entity, if applicable.
      $id = $row->{$this->field_alias};

      // Node revisions need special loading.
      if ($this->view->base_table == 'node_revision') {
        $this->entities[$id] = \Drupal::entityManager()->getStorage('node')->loadRevision($id);
      }
      // For other entities we just create an array of ids to pass
      // to entity_load().
      else {
        $ids[$id] = $id;
      }
    }

    $base_tables = date_views_base_tables();
    $this->entity_type = $base_tables[$this->view->base_table];
    if (!empty($ids)) {
      $this->entities = \Drupal::entityManager()->getStorage($this->entity_type);
    }

    // Let the style know if a link to create a new date is required.
    $this->view->date_info->calendar_date_link = $this->options['calendar_date_link'];

    // Identify the date argument and fields that apply to this view.
    // Preload the Date Views field info for each field, keyed by the
    // field name, so we know how to retrieve field values from the cached node.
    $data = date_views_fields($this->view->base_table);
    $data = $data['name'];
    $date_fields = array();
    foreach ($this->view->argument as $handler) {
      if (date_views_handler_is_date($handler, 'argument')) {
        // If this is the complex Date argument, the date fields are stored in the handler options,
        // otherwise we are using the simple date field argument handler.
        if ($handler->definition['handler'] != 'date_views_argument_handler') {
          $alias = $handler->table_alias . '.' . $handler->field;
          $info = $data[$alias];
          $field_name  = str_replace(array('_value2', '_value'), '', $info['real_field_name']);
          $date_fields[$field_name] = $info;
        }
        else {
          foreach ($handler->options['date_fields'] as $alias) {
            // If this date field is unknown (as when it has been deleted), ignore it.
            if (!array_key_exists($alias, $data)) {
              continue;
            }
            $info = $data[$alias];
            $field_name  = str_replace(array('_value2', '_value'), '', $info['real_field_name']);

            // This is ugly and hacky but I can't figure out any generic way to
            // recognize that the node module is going to give some the revision timestamp
            // a different field name on the entity than the actual column name in the database.
            if ($this->view->base_table == 'node_revision' && $field_name == 'timestamp') {
              $field_name = 'revision_timestamp';
            }

            $date_fields[$field_name] = $info;
          }
        }
        $this->date_argument = $handler;
        $this->date_fields = $date_fields;
      }
    }

    // Get the language for this view.
    $this->language = $this->display->handler->get_option('field_language');
    $substitutions = views_views_query_substitutions($this->view);
    if (array_key_exists($this->language, $substitutions)) {
      $this->language = $substitutions[$this->language];
    }
  }

  function render($row) {
    global $base_url;
    $rows = array();
    $date_info = $this->date_argument->view->date_info;
    $id = $row->{$this->field_alias};

    if (!is_numeric($id)) {
      return $rows;
    }

    // There could be more than one date field in a view
    // so iterate through all of them to find the right values
    // for this view result.
    foreach ($this->date_fields as $field_name => $info) {

      // Load the specified node:
      // We have to clone this or nodes on other views on this page,
      // like an Upcoming block on the same page as a calendar view,
      // will end up acquiring the values we set here.
      $entity = clone($this->entities[$id]);

      if (empty($entity)) {
        return $rows;
      }

      $table_name  = $info['table_name'];
      $delta_field = $info['delta_field'];
      $tz_handling = $info['tz_handling'];
      $tz_field    = $info['timezone_field'];
      $rrule_field = $info['rrule_field'];
      $is_field    = $info['is_field'];

      $info = \Drupal::entityManager()->getDefinition($this->entity_type);
      $this->id_field = $info['entity keys']['id'];
      $this->id = $entity->{$this->id_field};
      $this->type = !empty($info['entity keys']['bundle']) ? $info['entity keys']['bundle'] : $this->entity_type;
      $this->title = $entity->label();
      $uri = entity_uri($this->entity_type, $entity);
      $uri['options']['absolute'] = TRUE;
      // @FIXME
// url() expects a route name or an external URI.
// $this->url = url($uri['path'], $uri['options']);


      // Retrieve the field value(s) that matched our query from the cached node.
      // Find the date and set it to the right timezone.

      $entity->date_id = array();
      $item_start_date = NULL;
      $item_end_date   = NULL;
      $granularity = 'second';
      $increment = 1;
      if ($is_field) {

        // Set the date_id for the node, used to identify which field value to display for
        // fields that have multiple values. The theme expects it to be an array.
        $date_id = 'date_id_' . $field_name;
        $date_delta = 'date_delta_' . $field_name;
        if (isset($row->$date_id)) {
          $delta = $row->$date_delta;
          $entity->date_id = array('calendar.' . $row->$date_id . '.' . $field_name. '.' . $delta);
          $delta_field = $date_delta;
        }
        else {
          $delta = isset($row->$delta_field) ? $row->$delta_field : 0;
          $entity->date_id = array('calendar.' . $id . '.' . $field_name . '.' . $delta);
        }

        $items = field_get_items($this->entity_type, $entity, $field_name, $this->language);
        $item  = $items[$delta];
        $db_tz   = date_get_timezone_db($tz_handling, isset($item->$tz_field) ? $item->$tz_field : $date_info->display_timezone_name);
        $to_zone = date_get_timezone($tz_handling, isset($item->$tz_field) ? $item->$tz_field : $date_info->display_timezone_name);
        if (isset($item['value'])) {
          $item_start_date = new dateObject($item['value'], $db_tz);
          $item_end_date   = array_key_exists('value2', $item) ? new dateObject($item['value2'], $db_tz) : $item_start_date;
        }

        $cck_field = field_info_field($field_name);
        $instance = field_info_instance($this->entity_type, $field_name, $this->type);
        $granularity = date_granularity_precision($cck_field['settings']['granularity']);
        $increment = $instance['widget']['settings']['increment'];

      }
      elseif (!empty($entity->$field_name)) {
        $item = $entity->$field_name;
        $db_tz   = date_get_timezone_db($tz_handling, isset($item->$tz_field) ? $item->$tz_field : $date_info->display_timezone_name);
        $to_zone = date_get_timezone($tz_handling, isset($item->$tz_field) ? $item->$tz_field : $date_info->display_timezone_name);
        $item_start_date = new dateObject($item, $db_tz);
        $item_end_date   = $item_start_date;
        $entity->date_id = array('calendar.' . $id . '.' . $field_name . '.0');
      }

      // If we don't have a date value, go no further.
      if (empty($item_start_date)) {
        continue;
      }

      // Set the item date to the proper display timezone;
      $item_start_date->setTimezone(new dateTimezone($to_zone));
      $item_end_date->setTimezone(new dateTimezone($to_zone));

      $event = new stdClass();
      $event->id = $this->id;
      $event->title = $this->title;
      $event->type = $this->type;
      $event->date_start = $item_start_date;
      $event->date_end = $item_end_date;
      $event->db_tz = $db_tz;
      $event->to_zone = $to_zone;
      $event->granularity = $granularity;
      $event->increment = $increment;
      $event->field = $is_field ? $item : NULL;
      $event->url = $this->url;
      $event->row = $row;
      $event->entity = $entity;
      $event->stripe = array();
      $event->stripe_label = array();

      // All calendar row plugins should provide a date_id that the theme can use.
      $event->date_id = $entity->date_id[0];

      // We are working with an array of partially rendered items
      // as we process the calendar, so we can group and organize them.
      // At the end of our processing we'll need to swap in the fully formatted
      // display of the row. We save it here and switch it in
      // template_preprocess_calendar_item().
      // @FIXME
// theme() has been renamed to _theme() and should NEVER be called directly.
// Calling _theme() directly can alter the expected output and potentially
// introduce security issues (see https://www.drupal.org/node/2195739). You
// should use renderable arrays instead.
// 
// 
// @see https://www.drupal.org/node/2195739
// $event->rendered = theme($this->theme_functions(),
//       array(
//         'view' => $this->view,
//         'options' => $this->options,
//         'row' => $row,
//         'field_alias' => isset($this->field_alias) ? $this->field_alias : '',
//       ));


      $entities = $this->explode_values($event);
      foreach ($entities as $entity) {
        switch ($this->options['colors']['legend']) {
          case 'type':
            $this->calendar_node_type_stripe($entity);
            break;
          case 'taxonomy':
            $this->calendar_taxonomy_stripe($entity);
            break;
        }
        $rows[] = $entity;
      }

    }

    return $rows;
  }

  function explode_values($event) {
    $rows = array();

    $date_info = $this->date_argument->view->date_info;
    $item_start_date = $event->date_start;
    $item_end_date = $event->date_end;
    $to_zone = $event->to_zone;
    $db_tz = $event->db_tz;
    $granularity = $event->granularity;
    $increment = $event->increment;

    // Now that we have an 'entity' for each view result, we need
    // to remove anything outside the view date range,
    // and possibly create additional nodes so that we have a 'node'
    // for each day that this item occupies in this view.
    $now = max($date_info->min_zone_string, $item_start_date->format(DATE_FORMAT_DATE));
    $to  = min($date_info->max_zone_string, $item_end_date->format(DATE_FORMAT_DATE));
    $next = new DateObject($now . ' 00:00:00', $date_info->display_timezone);
    if ($date_info->display_timezone_name != $to_zone) {
      // Make $start and $end (derived from $node) use the timezone $to_zone, just as the original dates do.
      date_timezone_set($next, timezone_open($to_zone));
    }
    if (empty($to) || $now > $to) {
      $to = $now;
    }
    // $now and $next are midnight (in display timezone) on the first day where node will occur.
    // $to is midnight on the last day where node will occur.
    // All three were limited by the min-max date range of the view.
    $pos = 0;
    while (!empty($now) && $now <= $to) {
      $entity = clone($event);

      // Get start and end of current day.
      $start = $next->format(DATE_FORMAT_DATETIME);
      date_modify($next, '+1 day');
      date_modify($next, '-1 second');
      $end = $next->format(DATE_FORMAT_DATETIME);

      // Get start and end of item, formatted the same way.
      $item_start = $item_start_date->format(DATE_FORMAT_DATETIME);
      $item_end = $item_end_date->format(DATE_FORMAT_DATETIME);

      // Get intersection of current day and the node value's duration (as strings in $to_zone timezone).
      $entity->calendar_start = $item_start < $start ? $start : $item_start;
      $entity->calendar_end = !empty($item_end) ? ($item_end > $end ? $end : $item_end) : NULL;
//      $entity->calendar_end = !empty($item_end) ? ($item_end > $end ? $end : $item_end) : $node->calendar_start;

      // Make date objects
      $entity->calendar_start_date = date_create($entity->calendar_start, timezone_open($to_zone));
      $entity->calendar_end_date = date_create($entity->calendar_end, timezone_open($to_zone));

      // Change string timezones into
      // calendar_start and calendar_end are UTC dates as formatted strings
      $entity->calendar_start = date_format($entity->calendar_start_date, DATE_FORMAT_DATETIME);
      $entity->calendar_end = date_format($entity->calendar_end_date, DATE_FORMAT_DATETIME);
      $entity->calendar_all_day = date_is_all_day($entity->calendar_start, $entity->calendar_end, $granularity, $increment);

      unset($entity->calendar_fields);
      if (isset($entity) && (empty($entity->calendar_start))) {
        // if no date for the node and no date in the item
        // there is no way to display it on the calendar
        unset($entity);
      }
      else {
        $entity->date_id .= '.' . $pos;

        $rows[] = $entity;
        unset($entity);
      }
      date_modify($next, '+1 second');
      $now = date_format($next, DATE_FORMAT_DATE);
      $pos++;

    }
    return $rows;
  }

  /**
   * Create a stripe base on node type.
   */
  function calendar_node_type_stripe(&$result) {
    $colors = isset($this->options['colors']['calendar_colors_type']) ? $this->options['colors']['calendar_colors_type'] : array();
    if (empty($colors)) {
      return;
    }
    $entity = $result->entity;
    if (empty($entity->type)) {
      return;
    }

    $type_names = node_type_get_names();
    $type = $entity->type;
    $label = '';
    $stripe = '';
    if (array_key_exists($type, $type_names) || $colors[$type] == CALENDAR_EMPTY_STRIPE) {
      $label = $type_names[$type];
    }
    if (array_key_exists($type, $colors)) {
      $stripe = $colors[$type];
    }

    $result->stripe[] = $stripe;
    $result->stripe_label[] = $label;
    return;
  }

   /**
   * Create a stripe based on a taxonomy term.
   */

  function calendar_taxonomy_stripe(&$result) {
    $colors = isset($this->options['colors']['calendar_colors_taxonomy']) ? $this->options['colors']['calendar_colors_taxonomy'] : array();
    if (empty($colors)) {
      return;
    }

    $entity = $result->entity;
    $term_field_name = $this->options['colors']['taxonomy_field'];
    if ($terms_for_entity = field_get_items($this->view->base_table, $entity, $term_field_name)) {
      foreach ($terms_for_entity as $delta => $item) {
        $term_for_entity = \Drupal::entityManager()->getStorage("taxonomy_term")->load($item['tid']);
        if (!array_key_exists($term_for_entity->tid, $colors) || $colors[$term_for_entity->tid] == CALENDAR_EMPTY_STRIPE) {
          continue;
        }
        $result->stripe[] = $colors[$term_for_entity->tid];
        $result->stripe_label[] = $term_for_entity->name;
      }
    }

    return;
  }

}
