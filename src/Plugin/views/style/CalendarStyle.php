<?php

/**
 * @file
 * Contains \Drupal\calendar\Plugin\views\style\CalendarStyle.
 */

namespace Drupal\calendar\Plugin\views\style;

use Drupal\calendar\Plugin\views\row\CalendarRow;
use Drupal\Core\Form\FormStateInterface;
use Drupal\views\Plugin\views\style\StylePluginBase;

/**
 * Views style plugin for the Calendar module.
 *
 * @ingroup views_style_plugins
 *
 * @ViewsStyle(
 *   id = "calendar",
 *   title = @Translation("Calendar"),
 *   help = @Translation("Present view results as a Calendar."),
 *   theme = "calendar_style",
 *   display_types = {"normal"},
 *   even_empty = TRUE
 * )
 */
class CalendarStyle extends StylePluginBase {

  /**
   * Does the style plugin for itself support to add fields to it's output.
   *
   * @var bool
   */
  protected $usesFields = TRUE;

  /**
   * Does the style plugin allows to use style plugins.
   *
   * @var bool
   */
  protected $usesRowPlugin = TRUE;

  /**
   * Does the style plugin support grouping of rows.
   *
   * @var bool
   */
  protected $usesGrouping = FALSE;

  /**
   * {@inheritdoc}
   */
  protected function defineOptions() {
    $options = parent::defineOptions();

    $options['calendar_type'] = ['default' => 'month'];
    $options['name_size'] = ['default' => 3];
    $options['mini'] = ['default' => 0];
    $options['with_weekno'] = ['default' => 0];
    $options['multiday_theme'] = ['default' => 1];
    $options['theme_style'] = ['default' => 1];
    $options['max_items'] = ['default' => 0];
    $options['max_items_behavior'] = ['default' => 'more'];
    $options['groupby_times'] = ['default' => 'hour'];
    $options['groupby_times_custom'] = ['default' => ''];
    $options['groupby_field'] = ['default' => ''];
    $options['multiday_hidden'] = ['default' => []];

    return $options;
  }

  /**
   * {@inheritdoc}
   */
  public function buildOptionsForm(&$form, FormStateInterface $form_state) {
    parent::buildOptionsForm($form, $form_state);

    $form['calendar_type'] = [
      '#title' => $this->t('Calendar type'),
      '#type' => 'select',
      '#description' => $this->t('Select the calendar time period for this display.'),
      '#default_value' => $this->options['calendar_type'],
      '#options' => calendar_display_types(),
    ];
    $form['mini'] = [
      '#title' => $this->t('Display as mini calendar'),
      '#default_value' => $this->options['mini'],
      '#type' => 'radios',
      '#options' => [
        0 => $this->t('No'),
        1 => $this->t('Yes'),
      ],
      '#description' => $this->t('Display the mini style calendar, with no item details. Suitable for a calendar displayed in a block.'),
      '#dependency' => ['edit-style-options-calendar-type' => ['month']],
    ];
    $form['name_size'] = [
      '#title' => $this->t('Calendar day of week names'),
      '#default_value' => $this->options['name_size'],
      '#type' => 'radios',
      '#options' => [
        1 => $this->t('First letter of name'),
        2 => $this->t('First two letters of name'),
        3 => $this->t('Abbreviated name'),
        99 => $this->t('Full name'),
      ],
      '#description' => $this->t('The way day of week names should be displayed in a calendar.'),
      '#dependency' => [
        'edit-style-options-calendar-type' => ['month', 'week', 'year'],
      ],
    ];
    $form['with_weekno'] = [
      '#title' => $this->t('Show week numbers'),
      '#default_value' => $this->options['with_weekno'],
      '#type' => 'radios',
      '#options' => [
        0 => $this->t('No'),
        1 => $this->t('Yes'),
      ],
      '#description' => $this->t('Whether or not to show week numbers in the left column of calendar weeks and months.'),
      '#dependency' => ['edit-style-options-calendar-type' => ['month']],
    ];
    $form['max_items'] = [
      '#title' => $this->t('Maximum items'),
      '#type' => 'select',
      '#options' => [
        0 => $this->t('Unlimited'),
        1 => $this->formatPlural(1, '1 item', '@count items'),
        3 => $this->formatPlural(3, '1 item', '@count items'),
        5 => $this->formatPlural(5, '1 item', '@count items'),
        10 => $this->formatPlural(10, '1 item', '@count items'),
      ],
      '#default_value' => $this->options['calendar_type'] != 'day' ? $this->options['max_items'] : 0,
      '#description' => $this->t('Maximum number of items to show in calendar cells, used to keep the calendar from expanding to a huge size when there are lots of items in one day.'),
      '#dependency' => ['edit-style-options-calendar-type' => ['month']],
    ];
    $form['max_items_behavior'] = [
      '#title' => $this->t('Too many items'),
      '#type' => 'select',
      '#options' => [
        'more' => $this->t("Show maximum, add 'more' link"),
        'hide' => $this->t('Hide all, add link to day'),
      ],
      '#default_value' => $this->options['calendar_type'] != 'day' ? $this->options['max_items_behavior'] : 'more',
      '#description' => $this->t('Behavior when there are more than the above number of items in a single day. When there more items than this limit, a link to the day view will be displayed.'),
      '#dependency' => ['edit-style-options-calendar-type' => ['month']],
    ];
    $form['groupby_times'] = [
      '#title' => $this->t('Time grouping'),
      '#type' => 'select',
      '#default_value' => $this->options['groupby_times'],
      '#description' => $this->t("Group items together into time periods based on their start time."),
      '#options' => [
        '' => $this->t('None'),
        'hour' => $this->t('Hour'),
        'half' => $this->t('Half hour'),
        'custom' => $this->t('Custom'),
      ],
      '#dependency' => [
        'edit-style-options-calendar-type' => ['day', 'week'],
      ],
    ];
    $form['groupby_times_custom'] = [
      '#title' => $this->t('Custom time grouping'),
      '#type' => 'textarea',
      '#default_value' => $this->options['groupby_times_custom'],
      '#description' => $this->t("When choosing the 'custom' Time grouping option above, create custom time period groupings as a comma-separated list of 24-hour times in the format HH:MM:SS, like '00:00:00,08:00:00,18:00:00'. Be sure to start with '00:00:00'. All items after the last time will go in the final group."),
      '#dependency' => ['edit-style-options-groupby-times' => ['custom']],
    ];
    $form['theme_style'] = [
      '#title' => $this->t('Overlapping time style'),
      '#default_value' => $this->options['theme_style'],
      '#type' => 'select',
      '#options' => [
        0 => $this->t('Do not display overlapping items'),
        1 => $this->t('Display overlapping items, with scrolling'),
        2 => $this->t('Display overlapping items, no scrolling'),
      ],
      '#description' => $this->t('Select whether calendar items are displayed as overlapping items. Use scrolling to shrink the window and focus on the selected items, or omit scrolling to display the whole day. This only works if hour or half hour time grouping is chosen!'),
      '#dependency' => [
        'edit-style-options-calendar-type' => ['day', 'week'],
      ],
    ];

    // Create a list of fields that are available for grouping.
    $field_options = [];
    $fields = $this->view->display_handler->getOption('fields');
    foreach ($fields as $field_name => $field) {
      $field_options[$field_name] = $field['field'];
    }
    $form['groupby_field'] = [
      '#title' => $this->t('Field grouping'),
      '#type' => 'select',
      '#default_value' => $this->options['groupby_field'],
      '#description' => $this->t("Optionally group items into columns by a field value, for instance select the content type to show items for each content type in their own column, or use a location field to organize items into columns by location. NOTE! This is incompatible with the overlapping style option."),
      '#options' => ['' => ''] + $field_options,
      '#dependency' => ['edit-style-options-calendar-type' => ['day']],
    ];
    $form['multiday_theme'] = [
      '#title' => $this->t('Multi-day style'),
      '#default_value' => $this->options['multiday_theme'],
      '#type' => 'select',
      '#options' => [
        0 => $this->t('Display multi-day item as a single column'),
        1 => $this->t('Display multi-day item as a multiple column row')
      ],
      '#description' => $this->t('If selected, items which span multiple days will displayed as a multi-column row.  If not selected, items will be displayed as an individual column.'),
      '#dependency' => [
        'edit-style-options-calendar-type' => ['month', 'week'],
      ],
    ];
    $form['multiday_hidden'] = [
      '#title' => $this->t('Fields to hide in Multi-day rows'),
      '#default_value' => $this->options['multiday_hidden'],
      '#type' => 'checkboxes',
      '#options' => $field_options,
      '#description' => $this->t('Choose fields to hide when displayed in multi-day rows. Usually you only want to see the title or Colorbox selector in multi-day rows and would hide all other fields.'),
      '#dependency' => [
        'edit-style-options-calendar-type' => ['month', 'week', 'day'],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function validateOptionsForm(&$form, FormStateInterface $form_state) {
    $groupby_times = $form_state->getValue(['style_options', 'groupby_times']);
    if ($groupby_times == 'custom' && $form_state->isValueEmpty(['style_options', 'groupby_times_custom'])) {
      $form_state->setErrorByName('groupby_times_custom', $this->t('Custom groupby times cannot be empty.'));
    }
    if ((!$form_state->isValueEmpty(['style_options', 'theme_style']) && empty($groupby_times)) || !in_array($groupby_times, ['hour', 'half'])) {
      $form_state->setErrorByName('theme_style', $this->t('Overlapping items only work with hour or half hour groupby times.'));
    }
    if (!$form_state->isValueEmpty(['style_options', 'theme_style']) && !$form_state->isValueEmpty(['style_options', 'group_by_field'])) {
      $form_state->setErrorByName('theme_style', $this->t('ou cannot use overlapping items and also try to group by a field value.'));
    }
    if ($groupby_times != 'custom') {
      $form_state->setValue(['style_options', 'groupby_times_custom'], NULL);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitOptionsForm(&$form, FormStateInterface $form_state) {
    $multiday_hidden = $form_state->getValue(['style_options', 'multiday_hidden']);
    $form_state->setValue(['style_options', 'multiday_hidden'], array_filter($multiday_hidden));
  }

  /**
   * Helper function to find the date argument handler for this view.
   */
  protected function dateArgumentHandler() {
    // @todo Fix this, check core/modules/datetime/datetime.views.inc.
//    $i = 0;
//    foreach ($this->view->argument as $name => $handler) {
//      if (date_views_handler_is_date($handler, 'argument')) {
//        $this->date_info->date_arg_pos = $i;
//        return $handler;
//      }
//      $i++;
//    }
    return FALSE;
  }

  /**
   * Inspect argument and view information to see which calendar period we
   * should show. The argument tells us what to use if there is no value, the
   * view args tell us what to use if there are values.
   */
  protected function granularity() {
    // @todo Document this.
    if (!$handler = $this->dateArgumentHandler()) {
      return 'month';
    }
    $default_granularity = !empty($handler) && !empty($handler->granularity) ? $handler->granularity : 'month';
    $wildcard = !empty($handler) ? $handler->options['exception']['value'] : '';
    $argument = $handler->argument;

    // @todo Anything else we need to do for 'all' arguments?
    if ($argument == $wildcard) {
      $this->view_granularity = $default_granularity;
    }
    elseif (!empty($argument)) {
      module_load_include('inc', 'date_api', 'date_api_sql');

      $date_handler = new date_sql_handler();
      $this->view_granularity = $date_handler->arg_granularity($argument);
    }
    else {
      $this->view_granularity = $default_granularity;
    }
    return $this->view_granularity;
  }

  /**
   * @todo Document this.
   */
  protected function hasCalendarRowPlugin() {
    return $this->view->rowPlugin instanceof CalendarRow;
  }
}
