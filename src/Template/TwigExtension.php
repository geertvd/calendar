<?php
/**
 * @file
 * Contains \Drupal\calendar\Template\TwigExtension.
 */

namespace Drupal\calendar\Template;

/**
 * A class providing Calendar Twig extensions.
 */
class TwigExtension extends \Twig_Extension {

  /**
   * {@inheritdoc}
   */
  public function getName() {
    return 'calendar';
  }

  /**
   * {@inheritdoc}
   */
  public function getFilters() {
    return [
      new \Twig_SimpleFilter('calendar_stripe', [$this, 'getCalendarStripe'], array('is_safe' => array('html'))),
    ];
  }

  /**
   * Adds a striped background to the passed event.
   *
   * @param \Drupal\calendar\CalendarEvent $event
   */
  public function getCalendarStripe($event) {
    // @TODO implement

//    $item = $vars['item'];
//    if (empty($item->stripe) || (!count($item->stripe))) {
//      return;
//    }
//    $output = '';
//    if (is_array($item->stripe_label)) {
//      foreach ($item->stripe_label as $k => $stripe_label) {
//        if (!empty($item->stripe[$k]) && !empty($stripe_label)) {
//          $output .= '<div style="background-color:' . $item->stripe[$k] . ';color:' . $item->stripe[$k] . '" class="stripe" title="Key: ' . $item->stripe_label[$k] . '">&nbsp;</div>' . "\n";
//        }
//      }
//    }
//    return $output;

    return '<div style="background-color:' . 'red' . ';color:' . 'yellow' . '" class="stripe" title="Key: ' . 'label test' . '">&nbsp;</div>' . "\n";
  }
}