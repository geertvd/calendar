<?php
//$Id$
/**
 *  Implementation of hook_views_query()
 *  
 *  Handle the date_popup calendar goto date.
 */
function calendar_views_query_alter(&$query, &$view) {
  // Check if a new date has been selected and if so redirect.
  if (isset($_POST['calendar_goto'])) {
    require_once('./'. drupal_get_path('module', 'date_api') .'/date_api_elements.inc');
    $format = date_limit_format(variable_get('date_format_short', 'm/d/Y - H:i'), array('year', 'month', 'day'));
    $date = date_convert_from_custom($_POST['calendar_goto']['date'], $format);  
    switch ($_POST['calendar_type']) {
      case 'year':
        $parts = array(
          'year' => date_pad(date_part_extract($date, 'year'), 4),
        );
        break; 
      case 'month':
        $parts = array(
          'year' => date_pad(date_part_extract($date, 'year'), 4),
          'month' => date_pad(date_part_extract($date, 'month')),
        );
        break; 
      case 'week':
        $parts = array(
          'year' => date_pad(date_part_extract($date, 'year'), 4),
          'week' => 'W'. date_pad(date_week($date)),
        );
        break; 
      default:
        $parts = array(
          'year' => date_pad(date_part_extract($date, 'year'), 4),
          'month' => date_pad(date_part_extract($date, 'month')),
          'day' => date_pad(date_part_extract($date, 'day')),
        );
        break; 
        
    }
    drupal_goto($view->url .'/'. implode('/', $parts));
    drupal_exit();
  }
}

function calendar_views_pre_view(&$view, &$display_id, &$args) {
  //
}

/**
 * @file
 * 
 * Creates calendar displays of Views results.
 * 
 * Create a new calendar by enabling or cloning the default calendar,
 * changing the date argument to use the correct date field(s), and setting
 * up the year, month, day, week, and block views with the desired styles 
 * and fields.
 * 
 * Unlike previous versions of the Calendar module, there is just a single
 * Date argument instead of year, month, and day arguments. The argument
 * value will be YYYY-MM-DD for a day, YYYY-MM for a month, YYYY for a
 * year, and YYYY-W99 for a week. There is a default option to set the 
 * argument to the current date when the argument is empty.
 * 
 * A calendar display creates calendar navigation and links to 
 * multiple displays for the year, month, day, or week views. The actual
 * displays are created by attaching calendar views that use whatever
 * styles are desired for those pages. 
 * 
 * Calendar views are attachments to create the year, month, day,
 * and week displays. They can be set to use any style, either a
 * calendar style or any other Views style, like teasers or lists.
 * If you don't want to use one of them, don't attach it to
 * anything. Only the attached views will show up in the calendar.
 * 
 * A calendar block will create a calendar block for the
 * view results. Attach a block view to the block and set up the
 * desired style in the block view. 
 */
/**
 * Implementation of hook_views_plugins
 */
function calendar_views_plugins() {
  $path = drupal_get_path('module', 'calendar');
  $views_path = drupal_get_path('module', 'views');
  require_once "./$path/theme/theme.inc";

  $data = array(
    'module' => 'calendar', // This just tells our themes are elsewhere.
    'display' => array(
      // Parents are not really displays, just needed so the files can
      // be included.
      'parent' => array(
        'no ui' => TRUE,
        'handler' => 'views_plugin_display',
        'path' => "$views_path/plugins",
        'parent' => '',
      ),
      'page' => array(
        'no ui' => TRUE,
        'handler' => 'views_plugin_display_page',
        'path' => "$views_path/plugins",
        'parent' => 'parent',
      ),
      'block' => array(
        'no ui' => TRUE,
        'handler' => 'views_plugin_display_block',
        'path' => "$views_path/plugins",
        'parent' => 'parent',
      ),
      'attachment' => array(
        'no ui' => TRUE,
        'handler' => 'views_plugin_display_attachment',
        'path' => "$views_path/plugins",
        'parent' => 'parent',
      ),
      'calendar_attachment' => array(
        'handler' => 'calendar_plugin_display_attachment',
        'path' => "$path/includes",
        'parent' => 'attachment',
        'no ui' => TRUE,
        ),
      // Main calendar display plugin.
      'calendar' => array(
        'title' => t('Calendar page'),
        'help' => t('Calendar page display. Attach one or more of the Year/Month/Day/Week views to this page.'),
        'handler' => 'calendar_plugin_display_page',
        'path' => "$path/includes",
        'parent' => 'page',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'uses hook menu' => TRUE,
        'use ajax' => TRUE,
        'use pager' => FALSE,
        'accept attachments' => TRUE,
        'admin' => t('Calendar page'),
        'help topic' => 'getting-started',
        'js' => array(
          'misc/farbtastic/farbtastic.js', 
          drupal_get_path('module', 'calendar') .'/js/calendar_colorpicker.js',
          ),
      ),
      // Calendar block display plugin.
      'calendar_block' => array(
        'title' => t('Calendar block'),
        'help' => t('Calendar block display. Attach a block view to this block'),
        'handler' => 'calendar_plugin_display_block',
        'path' => "$path/includes",
        'parent' => 'block',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'uses hook block' => TRUE,
        'use ajax' => TRUE,
        'use pager' => FALSE,
        'use more' => TRUE,
        'accept attachments' => TRUE,
        'admin' => t('Calendar block'),
        'help topic' => 'getting-started',
      ),
      // Display plugins for calendar displays.
      'calendar_year' => array(
        'title' => t('Year view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_year',
        'path' => "$path/includes",
        'file' => 'calendar_plugin_display_attachment.inc',
        'parent' => 'calendar_attachment',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page year'),
        'help topic' => 'getting-started',
      ),
      'calendar_month' => array(
        'title' => t('Month view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_month',
        'path' => "$path/includes",
        'file' => 'calendar_plugin_display_attachment.inc',
        'parent' => 'calendar_attachment',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page month'),
        'help topic' => 'getting-started',
      ),
      'calendar_day' => array(
        'title' => t('Day view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_day',
        'path' => "$path/includes",
        'file' => 'calendar_plugin_display_attachment.inc',
        'parent' => 'calendar_attachment',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page day'),
        'help topic' => 'getting-started',
      ),
      'calendar_week' => array(
        'title' => t('Week view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_week',
        'path' => "$path/includes",
        'file' => 'calendar_plugin_display_attachment.inc',
        'parent' => 'calendar_attachment',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar page week'),
        'help topic' => 'getting-started',
      ),
      'calendar_block_view' => array(
        'title' => t('Block view'),
        'help' => t('This view can use any style you choose.'),
        'handler' => 'calendar_plugin_block',
        'path' => "$path/includes",
        'file' => 'calendar_plugin_display_attachment.inc',
        'parent' => 'calendar_attachment',
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'admin' => t('Calendar block view'),
        'help topic' => 'getting-started',
      ),
    ),
    'style' => array(
      'parent' => array(
        // this isn't really a display but is necessary so the file can
        // be included.
        'no ui' => TRUE,
        'handler' => 'views_plugin_style',
        'path' => "$views_path/plugins",
        'parent' => '',
      ),
      // Style plugin for the navigation.
      'calendar_nav' => array(
        'title' => t('Calendar navigation'),
        'help' => t('Creates back/next navigation and calendar links.'),
        'handler' => 'calendar_plugin_style',
        'path' => "$path/includes",
        'parent' => 'parent',
        'theme' => 'calendar_main',
        'theme file' => 'theme.inc',
        'theme path' => "$path/theme",
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => FALSE,
        'type' => 'calendar', // Only used on calendar page or block displays.
        'even empty' => TRUE,
      ),
      'calendar_style' => array(
        'title' => t('Calendar'),
        'help' => t('Displays Views results in a calendar.'),
        'handler' => 'calendar_view_plugin_style',
        'path' => "$path/includes",
        'parent' => 'calendar_nav',
        'theme' => 'calendar_month',
        'theme file' => 'theme.inc',
        'theme path' => "$path/theme",
        'additional themes' => array(
          'calendar_year' => 'style',
          'calendar_day' => 'style',
          'calendar_week' => 'style',
          'calendar_mini' => 'style',
          ),
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
  );
  return $data;
}