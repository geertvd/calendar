<?php
//$Id$
/**
 * @file
 * Setup and admin functions.
 *
 * Moved to a separate file so they're not parsed when not needed.
 */
/**
 * Setup Calendar feeds.
 *
 * @todo - control of the stripe color is not yet implemented.
 */
function _calendar_ical_setup_form($view_name) {
  include_once(drupal_get_path('module', 'date_api') .'/date_api_ical.inc');

  $form = array();
  $view = views_get_view($view_name);

  // Create an example of all the stripe colors below the form.
  for ($i = 0; $i < 10; $i++) {
    $node = new stdClass();
    $node->stripe = $i;
    $stripes[$i] = '<div style="width:20px;">'. $i .'</div>'.
      '<div class="calendar" style="width:130px;float:left;">'. theme('calendar_stripe_stripe', $node) .'</div>';
  }
  $form['#suffix'] = t('<h3>Stripe options</h3>') . implode($stripes);

  $period = drupal_map_assoc(array(0, 3600, 10800, 21600, 32400, 43200, 86400, 172800, 259200, 604800, 1209600, 2419200, 4838400, 9676800), 'format_interval');
  $form['calendar_ical_expire_'. $view->name] = array(
    '#type' => 'select', '#title' => t('Expire iCal cache'),
    '#default_value' => variable_get('calendar_ical_expire_'. $view->name, 9676800), '#options' => $period,
    '#description' => t('iCal feeds are cached to improve performance. Set an expiration time for cached feeds.')
    );

  $empty_feed = array(0 => array('name' => '', 'url' => '', 'type' => 'ical', 'stripe' => 0));
  $form[$view->name] = array(
    '#type' => 'fieldset',
    '#title' => t('iCal Feeds'),
    '#description' => t('Use this section to set up iCal feeds that should be displayed in this calendar. They will be shown along with any internal items that match the calendar criteria.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
    );

  // One empty input form will be added after any existing items.
  $view_feeds = array_merge((array) variable_get('calendar_feeds_'. $view->name, $empty_feed), $empty_feed);
  foreach ($view_feeds as $delta => $feed) {
    $form[$view->name][$delta] = array(
      'type' => array(
        '#title' => t('Feed type'),
        '#type' => 'hidden',
        '#value' => 'ical',
      ),
      'name' => array(
        '#title' => t('Name'),
        '#type' => 'textfield',
        '#default_value' => $feed['name'],
        '#description' => t('The name of a feed to include in this calendar.'),
       ),
      'url' => array(
        '#title' => t('Url'),
        '#type' => 'textarea',
        '#rows' => 2,
        '#default_value' => $feed['url'],
        '#description' => t('The external feed url or internal file path and name. Change \'webcal://\' to \'http://\'.'),
      ),
      'stripe' => array(
        '#title' => t('Stripe'),
        '#type' => 'select',
        '#options' => range(0, 10),
        '#default_value' => $feed['stripe'],
        '#description' => t('The color stripe to use for this feed (not working yet).'),
        '#suffix' => '<hr>',
       ),
    );
  }
  $form['view_name'] = array(
    '#type' => 'hidden',
    '#value' => $view->name,
    );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    );
  return $form;
}

/**
 * Save requested values.
 */
function calendar_ical_setup_form_submit($form_id, $form_values) {
  $view_name = $form_values['view_name'];
  foreach ($form_values as $value_name => $value) {
    if ($value_name == 'calendar_ical_expire_'. $view_name) {
      variable_set('calendar_ical_expire_'. $view_name,  $value);
    }
    elseif (is_array($value)) {
      foreach ($value as $delta => $item) {
        // Don't save empty values.
        if (trim($item['url']) == '' || trim($item['name']) == '') {
          unset($value[$delta]);
        }
        else {
          // Replace 'webcal' protocol with http protocol.
          $item['url'] = str_replace('webcal:', 'http:', $item['url']);
          // Don't save invalid urls.
          $events = date_ical_import($item['url']);
          if (!is_array($events)) {
            unset($value[$delta]);
          }
          else {
            $value[$delta]['url'] = $item['url'];
          }

        }
      }
      variable_set('calendar_feeds_'. $value_name, $value);
     }
  }
  cache_clear_all('calendar_feeds_'. $view->name, calendar_ical_cache(), TRUE);
}
