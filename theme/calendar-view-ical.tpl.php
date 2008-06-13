<?php
// $Id$
/**
 * $calname
 *   The name of the calendar.
 * $site_timezone
 *   The name of the site timezone.
 * $events
 *   An array with the following information about each event:
 * 
 *   $event['uid'] - a unique id for the event (usually the url).
 *   $event['summary'] - the name of the event.
 *   $event['start'] - the formatted start date of the event.
 *   $event['end'] - the formatted end date of the event.
 *   $event['timezone'] - the formatted timezone name of the event, if any.
 *   $event['url'] - the url for the event.
 *   $event['location'] - the name of the event location.
 *   $event['description'] - a description of the event.
 */
?>
BEGIN:VCALENDAR
VERSION:2.0
METHOD:PUBLISH
X-WR-CALNAME: <?php print $calname ?> 
PRODID:-//Drupal iCal API//EN
<?php foreach($events as $event): ?>
BEGIN:VEVENT
UID:<?php print $event['uid'] ?> 
SUMMARY:<?php print $event['summary'] ?> 
DTSTAMP;TZID=<?php print $site_timezone ?>;VALUE=DATE-TIME:<?php print $current_date ?> 
DTSTART;<?php print $event['timezone'] ?>VALUE=DATE-TIME:<?php print $event['start'] ?> 
<?php if (!empty($event['end'])): ?>
DTEND;<?php print $event['timezone'] ?>VALUE=DATE-TIME:<?php print $event['end'] ?> 
<?php endif; ?>
<?php if (!empty($event['url'])): ?>
URL;VALUE=URI:<?php print $event['url'] ?> 
<?php endif; ?>
<?php if (!empty($event['location'])): ?> 
LOCATION:<?php print $event['location'] ?> 
<?php endif; ?>
<?php if (!empty($event['description'])) : ?>
DESCRIPTION:<?php print $event['description'] ?>
<?php endif; ?>
END:VEVENT
<?php endforeach; ?>
END:VCALENDAR