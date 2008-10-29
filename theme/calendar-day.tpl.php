<?php
// $Id$
/**
 * @file
 * Template to display a view as a calendar day.
 * 
 * @see template_preprocess_calendar_day.
 *
 * $rows: The rendered data for this day.
 * $rows['date'] - the date for this day, formatted as YYYY-MM-DD.
 * $rows['datebox'] - the formatted datebox for this day.
 * $rows['empty'] - empty text for this day, if no items were found.
 * $rows['all_day'] - an array of formatted all day items.
 * $rows['items'] - an array of timed items for the day.
 * $rows['items']['hour'] - the formatted hour.
 * $rows['items']['ampm'] - the formatted ampm value, if any.
 * $rows['items']['values'] - an array of formatted items for this hour.
 * 
 * $view: The view.
 * $min_date_formatted: The minimum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * $max_date_formatted: The maximum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * 
 */
//dsm('Display: '. $display_type .': '. $min_date_formatted .' to '. $max_date_formatted);
//dsm($rows);
?>

<div class="calendar-calendar"><div class="day-view">
<table>
  <tbody>
    <tr>
      <td class="calendar-dayview-hour">
         <span class="calendar-hour"><?php print t('All day'); ?></span>
       </td>
       <td class="calendar-dayview-items">
         <div class="calendar"><div class="inner"><?php print implode($rows['all_day']); ?></div></div>
       </td>
    </tr>
    <?php foreach ($rows['items'] as $hour): ?>
    <tr>
      <td class="calendar-dayview-hour">
        <span class="calendar-hour"><?php print $hour['hour']; ?></span>
        <span class="calendar-ampm"><?php print $hour['ampm']; ?></span>
      </td>
      <td class="calendar-dayview-items">
        <div class="calendar"><div class="inner"><?php print implode($hour['values']); ?></div></div>
      </td>
    </tr>
   <?php endforeach; ?>   
  </tbody>
</table>
</div></div>