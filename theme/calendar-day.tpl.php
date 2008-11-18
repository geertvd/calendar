<?php
// $Id$
/**
 * @file
 * Template to display a view as a calendar day.
 * 
 * @see template_preprocess_calendar_day.
 *
 * $day_names: The day of week info for the table header.
 * $rows: The rendered data for this day.
 * $view: The view.
 * $min_date_formatted: The minimum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * $max_date_formatted: The maximum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * 
 * We use a table here just for consistency with the other views so the
 * styles and css will work the same for the day views as for the others.
 */
//dsm('Display: '. $display_type .': '. $min_date_formatted .' to '. $max_date_formatted);
//dsm($rows);
?>

<div class="calendar-calendar"><div class="day-view">
<table>
  <tbody>
    <?php foreach ($rows['items'] as $time): ?>
    <?php foreach ($time as $item): ?> 
    <tr>
       <td>
         <?php if (is_array($item)) print implode($item); ?>
       </td>
     </tr>
     <?php endforeach; ?>
     <?php endforeach; ?>
  </tbody>
</table>
</div></div>