<?php
// $Id$
/**
 * @file 
 * Template to display a the date box in a calendar.
 *
 * - $view: The view.
 * - $calendar_type: The type of calendar this box is in -- year, month, day, or week.
 * - $class: The class for this box -- mini-on, mini-off, or day.
 * - $day:  The day of the month.
 * - $date: The current date, in the form YYYY-MM-DD.
 * - $link: A formatted link to the calendar day view for this day.
 * - $url:  The url to the calendar day view for this day.
 * - $selected: Whether or not this day has any items.
 * - $items: An array of items for this day.
 */
?>
<div class="<?php print $class; ?>"> <?php print $link; ?> </div>