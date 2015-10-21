<?php
/**
 * @file
 * Contains \Drupal\calendar\CalendarHelper.
 */
namespace Drupal\calendar;

use Drupal\Core\Datetime\DateHelper;
use Drupal\views\Views;
use Drupal\Component\Utility\Unicode;

/**
 * Defines Gregorian Calendar date values.
 */
class CalendarHelper extends DateHelper {

  /**
   * Formats the weekday information into a table header format.
   *
   * @return array
   *   An array with weekday table header data.
   */
  public static function weekHeader($view) {
    $nameSize = $view->styleInfo->getNameSize();
    $len = isset($nameSize) ? $view->styleInfo->getNameSize() : (!empty($view->styleInfo->isMini()) ? 1 : 3);
    $with_week = !empty($view->styleInfo->isShowWeekNumbers());

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
      $row[] = ['header' => TRUE, 'class' => 'days week', 'data' => '', 'header_id' => 'Week'];
    }
    foreach ($untranslated_days as $delta => $day) {
      $label = $len < 3 ? Unicode::substr($translated_days[$delta], 0 , $len) : $translated_days[$delta];
      $row[] = ['header' => TRUE, 'class' => "days " . $day, 'data' => $label, 'header_id' => $full_translated_days[$delta]];
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

  /**
   * Return a list of all calendar views.
   *
   * @return array
   *   A list of all calendar views.
   */
  public static function listCalendarViews() {
    $calendar_views = [];
    $views = Views::getEnabledViews();
    foreach ($views as $view) {
      $ve = $view->getExecutable();
      $ve->initDisplay();
      foreach ($ve->displayHandlers->getConfiguration() as $display_id => $display) {
        if ($display_id != 'default' && $types = $ve->getStyle()->getPluginId() == 'calendar') {
          $index = $ve->id() . ':' . $display_id;
          $calendar_views[$index] = ucfirst($ve->id()) . ' ' . strtolower($display['display_title']) . ' [' . $ve->id() . ':' . $display['id'] . ']';
        }
      }
    }
    return $calendar_views;
  }

  /**
   * Computes difference between two days using a given measure.
   *
   * @param \DateTime $start_date
   *   The start date.
   * @param \DateTime $stop_date
   *   The stop date.
   * @param string $measure
   *   (optional) A granularity date part. Defaults to 'seconds'.
   * @param boolean $absolute
   *   (optional) Indicate whether the absolute value of the difference should
   *   be returned or if the sign should be retained. Defaults to TRUE.
   *
   * @return int
   *   The difference between the 2 dates in the given measure.
   */
  public static function difference(\DateTime $start_date, \DateTime $stop_date, $measure = 'seconds', $absolute = TRUE) {
    // Create cloned objects or original dates will be impacted by the
    // date_modify() operations done in this code.
    $date1 = clone($start_date);
    $date2 = clone($stop_date);
    if (is_object($date1) && is_object($date2)) {
      $diff = $date2->format('U') - $date1->format('U');
      if ($diff == 0) {
        return 0;
      }
      elseif ($diff < 0 && $absolute) {
        // Make sure $date1 is the smaller date.
        $temp = $date2;
        $date2 = $date1;
        $date1 = $temp;
        $diff = $date2->format('U') - $date1->format('U');
      }
      $year_diff = intval($date2->format('Y') - $date1->format('Y'));
      switch ($measure) {
        // The easy cases first.
        case 'seconds':
          return $diff;

        case 'minutes':
          return $diff / 60;

        case 'hours':
          return $diff / 3600;

        case 'years':
          return $year_diff;

        case 'months':
          $format = 'n';
          $item1 = $date1->format($format);
          $item2 = $date2->format($format);
          if ($year_diff == 0) {
            return intval($item2 - $item1);
          }
          elseif ($year_diff < 0) {
            $item_diff = 0 - $item1;
            $item_diff -= intval((abs($year_diff) - 1) * 12);
            return $item_diff - (12 - $item2);
          }
          else {
            $item_diff = 12 - $item1;
            $item_diff += intval(($year_diff - 1) * 12);
            return $item_diff + $item2;
          }
          break;

        case 'days':
          $format = 'z';
          $item1 = $date1->format($format);
          $item2 = $date2->format($format);
          if ($year_diff == 0) {
            return intval($item2 - $item1);
          }
          elseif ($year_diff < 0) {
            $item_diff = 0 - $item1;
            for ($i = 1; $i < abs($year_diff); $i++) {
              $date1->modify('-1 year');
              // @TODO self::daysInYear() throws a warning when used with a
              // \DateTime object. See https://www.drupal.org/node/2596043
//              $item_diff -= self::daysInYear($date1);
              $item_diff -= 365;
            }
//            return $item_diff - (self::daysInYear($date2) - $item2);
            return $item_diff - (365 - $item2);
          }
          else {
            // @TODO self::daysInYear() throws a warning when used with a
            // \DateTime object. See https://www.drupal.org/node/2596043
//            $item_diff = self::daysInYear($date1) - $item1;
            $item_diff = 365 - $item1;
            for ($i = 1; $i < $year_diff; $i++) {
              $date1->modify('+1 year');
//              $item_diff += self::daysInYear($date1);
              $item_diff += 365;
            }
            return $item_diff + $item2;
          }
          break;

        case 'weeks':
          $week_diff = $date2->format('W') - $date1->format('W');
          $year_diff = $date2->format('o') - $date1->format('o');

          $sign = ($year_diff < 0) ? -1 : 1;

          for ($i = 1; $i <= abs($year_diff); $i++) {
            $date1->modify((($sign > 0) ? '+': '-').'1 year');
            $week_diff += (self::isoWeeksInYear($date1) * $sign);
          }
          return $week_diff;
      }
    }
    return NULL;
  }

  /**
   * Identifies the number of ISO weeks in a year for a date.
   *
   * December 28 is always in the last ISO week of the year.
   *
   * @param mixed $date
   *   (optional) The current date object, or a date string. Defaults to NULL.
   *
   * @return integer
   *   The number of ISO weeks in a year.
   */
  public static function isoWeeksInYear($date = NULL) {
    if (empty($date)) {
      $date = new \DateTime();
    }
    elseif (!is_object($date)) {
      $date = new \DateTime($date);
    }

    if (is_object($date)) {
      date_date_set($date, $date->format('Y'), 12, 28);
      return $date->format('W');
    }
    return NULL;
  }

  /**
   * @deprecated
   *   This is a copy of the date_is_all_day() function from the date_api
   *   module in D7.
   * @TODO figure out where this should live
   */
  public static function dateIsAllDay($string1, $string2, $granularity = 'second', $increment = 1) {
    if (empty($string1) || empty($string2)) {
      return FALSE;
    }
    elseif (!in_array($granularity, ['hour', 'minute', 'second'])) {
      return FALSE;
    }

    preg_match('/([0-9]{4}-[0-9]{2}-[0-9]{2}) (([0-9]{2}):([0-9]{2}):([0-9]{2}))/', $string1, $matches);
    $count = count($matches);
    $date1 = $count > 1 ? $matches[1] : '';
    $time1 = $count > 2 ? $matches[2] : '';
    $hour1 = $count > 3 ? intval($matches[3]) : 0;
    $min1 = $count > 4 ? intval($matches[4]) : 0;
    $sec1 = $count > 5 ? intval($matches[5]) : 0;
    preg_match('/([0-9]{4}-[0-9]{2}-[0-9]{2}) (([0-9]{2}):([0-9]{2}):([0-9]{2}))/', $string2, $matches);
    $count = count($matches);
    $date2 = $count > 1 ? $matches[1] : '';
    $time2 = $count > 2 ? $matches[2] : '';
    $hour2 = $count > 3 ? intval($matches[3]) : 0;
    $min2 = $count > 4 ? intval($matches[4]) : 0;
    $sec2 = $count > 5 ? intval($matches[5]) : 0;
    if (empty($date1) || empty($date2)) {
      return FALSE;
    }
    if (empty($time1) || empty($time2)) {
      return FALSE;
    }

    $tmp = self::seconds('s', TRUE, $increment);
    $max_seconds = intval(array_pop($tmp));
    $tmp = self::minutes('i', TRUE, $increment);
    $max_minutes = intval(array_pop($tmp));

    // See if minutes and seconds are the maximum allowed for an increment or the
    // maximum possible (59), or 0.
    switch ($granularity) {
      case 'second':
        $min_match = $time1 == '00:00:00'
          || ($hour1 == 0 && $min1 == 0 && $sec1 == 0);
        $max_match = $time2 == '00:00:00'
          || ($hour2 == 23 && in_array($min2, [$max_minutes, 59]) && in_array($sec2, [$max_seconds, 59]))
          || ($hour1 == 0 && $hour2 == 0 && $min1 == 0 && $min2 == 0 && $sec1 == 0 && $sec2 == 0);
        break;
      case 'minute':
        $min_match = $time1 == '00:00:00'
          || ($hour1 == 0 && $min1 == 0);
        $max_match = $time2 == '00:00:00'
          || ($hour2 == 23 && in_array($min2, [$max_minutes, 59]))
          || ($hour1 == 0 && $hour2 == 0 && $min1 == 0 && $min2 == 0);
        break;
      case 'hour':
        $min_match = $time1 == '00:00:00'
          || ($hour1 == 0);
        $max_match = $time2 == '00:00:00'
          || ($hour2 == 23)
          || ($hour1 == 0 && $hour2 == 0);
        break;
      default:
        $min_match = TRUE;
        $max_match = FALSE;
    }

    if ($min_match && $max_match) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Calendar display types.
   */
  public static function displayTypes() {
    return ['year' => t('Year'), 'month' => t('Month'), 'day' => t('Day'), 'week' => t('Week')];
  }
}
