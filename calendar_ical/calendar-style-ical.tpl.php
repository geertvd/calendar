<?php
/**
 * $title
 *   The name of the calendar.
 */
if (empty($method)) {
  $method = 'PUBLISH';
}
?>
BEGIN:VCALENDAR
VERSION:2.0
METHOD:<?php print $method; ?>
X-WR-CALNAME;VALUE=TEXT:<?php print $title . "\r\n"; ?>
PRODID:-//Drupal iCal API//EN
<?php print $rows ?>
END:VCALENDAR
