<?php

/**
 * @file
 * Contains \Drupal\calendar\Plugin\views\argument\CalendarDate.
 */

namespace Drupal\calendar\Plugin\views\argument;

use Drupal\Core\Form\FormStateInterface;
use Drupal\views\Plugin\views\argument\Date;

/**
 * Calendar argument handler for a date.
 *
 * @ingroup views_argument_handlers
 *
 * @ViewsArgument("calendar_datetime")
 */
class CalendarDate extends Date {

  /**
   * {@inheritdoc}
   */
  protected function defineOptions() {
    $options = parent::defineOptions();

    $options['calendar'] = [
       'contains' => [
         'granularity' => ['default' => 'month'],
         'date_range' => ['default' => '-3:+3'],
       ],
    ];

    return $options;
  }

  /**
   * {@inheritdoc}
   */
  public function buildOptionsForm(&$form, FormStateInterface $form_state) {
    parent::buildOptionsForm($form, $form_state);

    $form['calendar'] = [
      '#type' => 'details',
      '#title' => $this->t('Calendar options'),
      '#open' => TRUE,
    ];

    $form['calendar']['granularity'] = [
      '#title' => $this->t('Granularity'),
      '#type' => 'radios',
      '#default_value' => $this->options['calendar']['granularity'],
      '#options' => [
        'year' => $this->t('Year'),
        'month' => $this->t('Month'),
        'week' => $this->t('Week'),
        'day' => $this->t('Day'),
        'hour' => $this->t('Hour'),
        'minute' => $this->t('Minute'),
        'second' => $this->t('Second'),
      ],
    ];

    $form['calendar']['date_range'] = [
      '#title' => $this->t('Date range'),
      '#type' => 'textfield',
      '#default_value' => $this->options['calendar']['date_range'],
    ];
  }
}
