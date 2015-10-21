<?php

/**
 * @file
 * Contains \Drupal\calendar\Plugin\views\row\Calendar.
 */

namespace Drupal\calendar\Plugin\views\row;

use Drupal\calendar\CalendarEvent;
use Drupal\calendar\CalendarHelper;
use Drupal\calendar\Plugin\views\argument\CalendarDate;
use Drupal\Core\Datetime\DateFormatter;
use Drupal\Core\Entity\Entity;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\TypedData\Plugin\DataType\DateTimeIso8601;
use Drupal\views\Plugin\views\display\DisplayPluginBase;
use Drupal\views\Plugin\views\display\DisplayRouterInterface;
use Drupal\views\Plugin\views\row\RowPluginBase;
use Drupal\views\ViewExecutable;
use Drupal\views\Views;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\Routing\RouteCollection;

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

  /**
   * @var \Drupal\Core\Datetime\DateFormatter $dateFormatter
   *   The date formatter service.
   */
  protected $dateFormatter;

  /**
   * @var $entityType
   *   The entity type being handled in the preRender() function.
   */
  protected $entityType;

  /**
   * @var $entities
   *   The entities loaded in the preRender() function.
   */
  protected $entities = [];

  /**
   * @var $dateFields
   *   todo document.
   */
  protected $dateFields = [];

  /**
   * @var \Drupal\views\Plugin\views\argument\Formula
   */
  protected $dateArgument;

  /**
   * {@inheritdoc}
   */
  public function init(ViewExecutable $view, DisplayPluginBase $display, array &$options = NULL) {
    parent::init($view, $display, $options);

    // TODO needed?
//     $this->base_table = $view->base_table;
//     $this->baseField = $view->base_field;
  }

  /**
   * Constructs a Calendar row plugin object.
   *
   * @param array $configuration
   *   A configuration array containing information about the plugin instance.
   * @param string $plugin_id
   *   The plugin_id for the plugin instance.
   * @param mixed $plugin_definition
   *   The plugin implementation definition.
   * @param \Drupal\Core\Datetime\DateFormatter $date_formatter
   *   The date formatter service.
   */
  public function __construct(array $configuration, $plugin_id, $plugin_definition, DateFormatter $date_formatter) {
    parent::__construct($configuration, $plugin_id, $plugin_definition);

    $this->dateFormatter = $date_formatter;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static($configuration, $plugin_id, $plugin_definition, $container->get('date.formatter'));
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
        'calendar_colors_type' => ['default' => []],
        'taxonomy_field' => ['default' => ''],
        'calendar_colors_vocabulary' => ['default' => []],
        'calendar_colors_taxonomy' => ['default' => []],
        'calendar_colors_group' => ['default' => []],
      ]
    ];
    return $options;
  }

  /**
   * {@inheritdoc}
   */
  public function buildOptionsForm(&$form, FormStateInterface $form_state) {
    parent::buildOptionsForm($form, $form_state);

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
          '#element_validate' => [[$this, 'validateHexColor']],
          '#prefix' => '<div class="calendar-colorpicker-wrapper">',
          '#suffix' => '<div class="calendar-colorpicker"></div></div>',
          '#attributes' => ['class' => ['edit-calendar-colorpicker']],
          '#attached' => [
            // Add Farbtastic color picker and the js to trigger it.
            'library' => [
              'calendar/calendar.colorpicker',
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
      $vocab_vids = [];
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

      $form['colors']['calendar_colors_vocabulary'] = [
        '#title' => t('Vocabulary Legend Types'),
        '#type' => 'value',
        '#value' => $vocab_vids,
      ];

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
            '#element_validate' => [[$this, 'validateHexColor']],
            '#prefix' => '<div class="calendar-colorpicker-wrapper">',
            '#suffix' => '<div class="calendar-colorpicker"></div></div>',
            '#attributes' => ['class' => ['edit-calendar-colorpicker']],
            '#attached' => [
              // Add Farbtastic color picker and the js to trigger it.
              'library' => [
                'calendar/calendar.colorpicker',
              ],
            ],
          ];
        }
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitOptionsForm(&$form, FormStateInterface $form_state) {
    parent::submitOptionsForm($form, $form_state);

    if ($this->view->getBaseTables()['node_field_data']) {
      // @todo figure out what the real default display is.
      $link_display = $this->view->getDisplay()->getOption('link_display');
      if (!empty($link_display)) {
        $view_id = $this->view->storage->id();
        $route = "view.$view_id.$link_display";

        // @todo uncomment after calendar_clear_link_path is fixed.
        //calendar_clear_link_path($path);
        if (!empty($form_state->getValue('row_options')['calendar_date_link'])) {
          $node_type = $form_state->getValue('row_options')['calendar_date_link'];
          calendar_set_link('node', $node_type, $route);
        }
      }
    }
  }

  /**
   *  Check to make sure the user has entered a valid 6 digit hex color.
   */
  public function validateHexColor($element, FormStateInterface $form_state) {
    if (!$element['#required'] && empty($element['#value'])) {
      return;
    }
    if (!preg_match('/^#(?:(?:[a-f\d]{3}){1,2})$/i', $element['#value'])) {
      $form_state->setError($element, $this->t("'@color' is not a valid hex color", array('@color' => $element['#value'])));
    }
    else {
      $form_state->setValueForElement($element, $element['#value']);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function preRender($result) {

    // Preload each entity used in this view from the cache. This provides all
    // the entity values relatively cheaply, and we don't need to do it
    // repeatedly for the same entity if there are multiple results for one
    // entity.
    $ids = [];
    /** @var $row \Drupal\views\ResultRow */
    foreach ($result as $row) {
      // Use the entity id as the key so we don't create more than one value per
      // entity.
      $entity = $row->_entity;

      // Node revisions need special loading.
      if (isset($this->view->getBaseTables()['node_revision'])) {
        $this->entities[$entity->id()] = \Drupal::entityManager()->getStorage('node')->loadRevision($entity->id());
      }
      else {
        $ids[$entity->id()] = $entity->id();
      }
    }

    $base_tables = Views::viewsData()->fetchBaseTables();
    $base_table = key($base_tables);
    $table_data = Views::viewsData()->get($base_table);
    $this->entityType = $table_data['table']['entity type'];

    if (!empty($ids)) {
      $this->entities = \Drupal::entityManager()->getStorage($this->entityType)->loadMultiple($ids);
    }

    // Let the style know if a link to create a new date is required.
    // @todo implement
    // see calendar_preprocess_date_views_pager() on some more info on how this
    // is used.
//    $this->view->dateInfo->setCalendarDateLink($this->options['calendar_date_link']);

    // Identify the date argument and fields that apply to this view. Preload
    // the Date Views field info for each field, keyed by the field name, so we
    // know how to retrieve field values from the cached node.
    // @todo don't hardcode $date_fields, use viewsData() or viewsDataHelper()

//    $data = date_views_fields($this->view->base_table);
//    $data = $data['name'];

    $data['name'] = 'node_field_data.created_year';
    $date_fields = [];
    /** @var $handler \Drupal\views\Plugin\views\argument\Formula */
    foreach ($this->view->getDisplay()->getHandlers('argument') as $handler) {
      if ($handler instanceof CalendarDate) {
        $date_fields[$handler->table] = $table_data[$handler->field];

        $this->dateArgument = $handler;
        $this->dateFields = $date_fields;
      }
    }
//
//    // Get the language for this view.
//    $this->language = $this->display->handler->get_option('field_language');
//    $substitutions = views_views_query_substitutions($this->view);
//    if (array_key_exists($this->language, $substitutions)) {
//      $this->language = $substitutions[$this->language];
//    }
  }

  /**
   * {@inheritdoc}
   */
  public function render($row) {
    /** @var \Drupal\calendar\CalendarDateInfo $dateInfo */
    $dateInfo = $this->dateArgument->view->dateInfo;
    $id = $row->_entity->id();

    if (!is_numeric($id)) {
      return [];
    }

    // There could be more than one date field in a view so iterate through all
    // of them to find the right values for this view result.
    foreach ($this->dateFields as $field_name => $info) {

      // Clone this entity so we can change it's values without altering other
      // occurrences of this entity on the same page, for example in an
      // "Upcoming" block.
      /** @var \Drupal\node\Entity\Node $entity */
      $entity = clone($this->entities[$id]);

      if (empty($entity)) {
        return [];
      }

      // @todo clean up
//      $table_name  = $info['table_name'];
//      $delta_field = $info['delta_field'];
//      $tz_handling = $info['tz_handling'];
//      $tz_field    = $info['timezone_field'];
//      $rrule_field = $info['rrule_field'];
//      $is_field    = $info['is_field'];

      // @todo needed?
      $info = \Drupal::entityManager()->getDefinition($this->entityType);

      $event = new CalendarEvent();
      $event->setTitle($entity->label());
      $event->setEntityId($entity->id());
      $event->setEntityTypeId($entity->getEntityType()->id());
      $event->setType($entity->getType());
      $event->setUrl($entity->url());

      // Retrieve the field value(s) that matched our query from the cached node.
      // Find the date and set it to the right timezone.
      $entity->date_id = [];
      $item_start_date = NULL;
      $item_end_date   = NULL;
      $granularity = 'second';
      $increment = 1;

      // @todo implement
      if (FALSE && $is_field) {

        // Set the date_id for the node, used to identify which field value to display for
        // fields that have multiple values. The theme expects it to be an array.
        $date_id = 'date_id_' . $field_name;
        $date_delta = 'date_delta_' . $field_name;
        if (isset($row->$date_id)) {
          $delta = $row->$date_delta;
          $entity->date_id = ['calendar.' . $row->$date_id . '.' . $field_name. '.' . $delta];
          $delta_field = $date_delta;
        }
        else {
          $delta = isset($row->$delta_field) ? $row->$delta_field : 0;
          $entity->date_id = ['calendar.' . $id . '.' . $field_name . '.' . $delta];
        }

        $items = field_get_items($this->entity_type, $entity, $field_name, $this->language);
        $item  = $items[$delta];
        $db_tz   = date_get_timezone_db($tz_handling, isset($item->$tz_field) ? $item->$tz_field : timezone_name_get($dateInfo->getTimezone()));
        $to_zone = date_get_timezone($tz_handling, isset($item->$tz_field)) ? $item->$tz_field : timezone_name_get($dateInfo->getTimezone());
        if (isset($item['value'])) {
          $item_start_date = new dateObject($item['value'], $db_tz);
          $item_end_date   = array_key_exists('value2', $item) ? new dateObject($item['value2'], $db_tz) : $item_start_date;
        }

        $cck_field = field_info_field($field_name);
        $instance = field_info_instance($this->entity_type, $field_name, $this->type);
        $granularity = date_granularity_precision($cck_field['settings']['granularity']);
        $increment = $instance['widget']['settings']['increment'];

      }
      // @todo implement
      elseif (FALSE && !empty($entity->$field_name)) {
        $item = $entity->$field_name;
        $db_tz   = date_get_timezone_db($tz_handling, isset($item->$tz_field) ? $item->$tz_field : timezone_name_get($dateInfo->getTimezone()));
        $to_zone = date_get_timezone($tz_handling, isset($item->$tz_field) ? $item->$tz_field : timezone_name_get($dateInfo->getTimezone()));
        $item_start_date = new dateObject($item, $db_tz);
        $item_end_date   = $item_start_date;
        $entity->date_id = ['calendar.' . $id . '.' . $field_name . '.0'];
      }

      // If we don't have a date value, go no further.
      // @todo remove this once the above loop is fixed
      $item_start_date = new \DateTime();
      $item_start_date->setTimestamp($entity->getCreatedTime());
      $item_start_date->setTime(0, 0, 0);
      $item_end_date = new \DateTime();
      $item_end_date->setTimestamp($entity->getCreatedTime() + 3600);
      $item_end_date->setTime(0, 0, 0);
      if (empty($item_start_date)) {
        continue;
      }

      // Set the item date to the proper display timezone;
      // @todo handle timezones
//      $item_start_date->setTimezone(new dateTimezone($to_zone));
//      $item_end_date->setTimezone(new dateTimezone($to_zone));

      $event->setStartDate($item_start_date);
      $event->setEndDate($item_end_date);
      $event->setTimezone(new \DateTimeZone(timezone_name_get($dateInfo->getTimezone())));

      // @todo remove while properties get transfered to the new object
//      $event_container = new stdClass();
//      $event_container->db_tz = $db_tz;
//      $event_container->to_zone = $to_zone;
//      $event_container->granularity = $granularity;
//      $event_container->increment = $increment;
//      $event_container->field = $is_field ? $item : NULL;
//      $event_container->row = $row;
//      $event_container->entity = $entity;

      // All calendar row plugins should provide a date_id that the theme can use.
      // @todo implement
//      $event_container->date_id = $entity->date_id[0];

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
            $this->nodeTypeStripe($entity);
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

  /**
   * @todo rename and document
   *
   * @param \Drupal\calendar\CalendarEvent $event
   * @return array
   */
  function explode_values($event) {
    $rows = [];

    $dateInfo = $this->dateArgument->view->dateInfo;
//    $item_start_date = $event->date_start;
//    $item_end_date = $event->date_end;
//    $to_zone = $event->to_zone;
//    $db_tz = $event->db_tz;
//    $granularity = $event->granularity;
//    $increment = $event->increment;

    // Now that we have an 'entity' for each view result, we need to remove
    // anything outside the view date range, and possibly create additional
    // nodes so that we have a 'node' for each day that this item occupies in
    // this view.
    // @TODO make this work with the CalendarDateInfo object
//    $now = max($dateInfo->min_zone_string, $this->dateFormatter->format($event->getStartDate()->getTimestamp(), 'Y-m-d'));
//    $to = min($dateInfo->max_zone_string, $this->dateFormatter->format($event->getEndDate()->getTimestamp(), 'Y-m-d'));
    $now = $this->dateFormatter->format($event->getStartDate()->getTimestamp(), 'Y-m-d');
    $to = $this->dateFormatter->format($event->getEndDate()->getTimestamp(), 'Y-m-d');
    $next = new \DateTime();
    $next->setTimestamp($event->getStartDate()->getTimestamp());

    if (timezone_name_get($this->dateArgument->view->dateInfo->getTimezone()) != $event->getTimezone()->getName()) {
      // Make $start and $end (derived from $node) use the timezone $to_zone,
      // just as the original dates do.
      $next->setTimezone($event->getTimezone());
    }

    if (empty($to) || $now > $to) {
      $to = $now;
    }

    // $now and $next are midnight (in display timezone) on the first day where node will occur.
    // $to is midnight on the last day where node will occur.
    // All three were limited by the min-max date range of the view.
    $position = 0;
    while (!empty($now) && $now <= $to) {
      /** @var $entity \Drupal\calendar\CalendarEvent */
      $entity = clone($event);

      // Get start and end of current day.
      $start = $this->dateFormatter->format($next->getTimestamp(), 'custom', 'Y-m-d H:i:s');
      $next->setTimestamp(strtotime(' +1 day -1 second', $next->getTimestamp()));
      $end = $this->dateFormatter->format($next->getTimestamp(), 'custom', 'Y-m-d H:i:s');

      // Get start and end of item, formatted the same way.
      $item_start = $this->dateFormatter->format($event->getStartDate()->getTimestamp(), 'custom', 'Y-m-d H:i:s');
      $item_end = $this->dateFormatter->format($event->getEndDate()->getTimestamp(), 'custom', 'Y-m-d H:i:s');

      // Get intersection of current day and the node value's duration (as
      // strings in $to_zone timezone).
      $start_string = $item_start < $start ? $start : $item_start;
      $entity->setStartDate(new \DateTime($start_string));
      $end_string = !empty($item_end) ? ($item_end > $end ? $end : $item_end) : NULL;
      $entity->setEndDate(new \DateTime($end_string));

      // @TODO don't hardcode granularity and increment
      $granularity = 'hour';
      $increment = 1;
      $entity->setAllDay(CalendarHelper::dateIsAllDay($entity->getStartDate()->format('Y-m-d H:i:s'), $entity->getEndDate()->format('Y-m-d H:i:s'), $granularity, $increment));

      $calendar_start = new \DateTime();
      $calendar_start->setTimestamp($entity->getStartDate()->getTimestamp());

//      unset($entity->calendar_fields);
      if (isset($entity) && (empty($calendar_start))) {
        // if no date for the node and no date in the item
        // there is no way to display it on the calendar
        unset($entity);
      }
      else {
//        $entity->date_id .= '.' . $position;
        $rows[] = $entity;
        unset($entity);
      }

      $next->setTimestamp(strtotime('+1 second', $next->getTimestamp()));
      $now = $this->dateFormatter->format($next->getTimestamp(), 'Y-m-d');
      $position++;
    }
    return $rows;
  }

  /**
   * Create a stripe base on node type.
   *
   * @param \Drupal\calendar\CalendarEvent $result
   *   The event result object.
   */
  function nodeTypeStripe(&$result) {
    $colors = isset($this->options['colors']['calendar_colors_type']) ? $this->options['colors']['calendar_colors_type'] : [];
    if (empty($colors)) {
      return;
    }

    $type_names = node_type_get_names();
    $type = $result->getType();
    $label = '';
    $stripe = '';
    if (array_key_exists($type, $type_names) || $colors[$type] == CALENDAR_EMPTY_STRIPE) {
      $label = $type_names[$type];
    }
    if (array_key_exists($type, $colors)) {
      $stripe = $colors[$type];
    }

    $result->setStripeLabels($result->getStripeLabels() + [$label]);
    $result->setStripeHexes($result->getStripeHexes() + [$stripe]);
  }

   /**
   * Create a stripe based on a taxonomy term.
    *
    * @todo rename and document
   */
  function calendar_taxonomy_stripe(&$result) {
    $colors = isset($this->options['colors']['calendar_colors_taxonomy']) ? $this->options['colors']['calendar_colors_taxonomy'] : [];
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
        $result->setStripeLabels($result->getStripeLabels() + [$colors[$term_for_entity->tid]]);
        $result->setStripeHexes($result->getStripeHexes() + [$term_for_entity->name]);
      }
    }

    return;
  }

}
