<?php
/**
 * @file
 * Contains \Drupal\calendar\Util\CalendarHelper.
 */
namespace Drupal\calendar\Util;

use Drupal\Core\Datetime\DateHelper;

/**
 * Defines Gregorian Calendar date values.
 */
class CalendarHelper extends DateHelper {

  /**
   * Formats the weekday information into a table header format.
   *
   * @ingroup event_support
   *
   * @return array
   *   An array with weekday table header data.
   */
  public static function weekHeader($view) {
    // @todo make this not hard-coded
    //  $len = isset($view->date_info->style_name_size) ? $view->date_info->style_name_size : (!empty($view->date_info->mini) ? 1 : 3);
    $len = 3;
    $with_week = !empty($view->date_info->style_with_weekno);

    // create week header
    $untranslated_days = self::untranslatedDays();
    $full_translated_days = self::weekDaysOrdered(self::weekDays(TRUE));
    if ($len == 99) {
      $translated_days = $full_translated_days;
    }
    else {
      $translated_days = self::weekDaysOrdered(self::weekDaysAbbr(TRUE));
    }
    if ($with_week) {
      $row[] = array('header' => TRUE, 'class' => "days week", 'data' => '&nbsp;', 'header_id' => 'Week');
    }
    foreach ($untranslated_days as $delta => $day) {
      $label = $len < 3 ? \Drupal\Component\Utility\Unicode::substr($translated_days[$delta], 0 , $len) : $translated_days[$delta];
      $row[] = array('header' => TRUE, 'class' => "days " . $day, 'data' => $label, 'header_id' => $full_translated_days[$delta]);
    }
    return $row;
  }


  /**
   * An array of untranslated day name abbreviations.
   *
   * The abbreviations are forced to lowercase and ordered appropriately for the
   * site setting for the first day of week.
   *
   * @return array
   *   The untranslated day abbreviation is used in css classes.
   */
  public static function untranslatedDays() {
    $untranslated_days = self::weekDaysOrdered(DateHelper::weekDaysUntranslated());
    foreach ($untranslated_days as $delta => $day) {
      $untranslated_days[$delta] = strtolower(substr($day, 0, 3));
    }
    return $untranslated_days;
  }

}
