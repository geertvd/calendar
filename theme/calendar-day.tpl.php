<?php
// $Id$
/**
 * @file
 * Template to display a view as a calendar day.
 * 
 * @see template_preprocess_calendar_day.
 *
 * $day_names: The day of week info for the table header.
 * $rows: An array of data for this day.
 * $view: The view.
 * $min_date_formatted: The minimum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * $max_date_formatted: The maximum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 */
//dsm('Display: '. $display_type .': '. $min_date_formatted .' to '. $max_date_formatted);
?>

<div class="calendar-calendar"><div class="day-view">
<table>
  <tbody>
     <tr>
       <td>
         <?php print $rows; ?>
       </td>
     </tr>
  </tbody>
</table>
</div></div>