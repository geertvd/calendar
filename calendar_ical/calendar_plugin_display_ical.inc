<?php
// $Id$
/**
 * The plugin that handles a feed, such as RSS or atom.
 *
 * For the most part, feeds are page displays but with some subtle differences.
 */
class calendar_plugin_display_ical extends views_plugin_display_feed {
  function get_style_type() { return 'ical'; }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    // It is very important to call the parent function here.
    parent::options_form($form, $form_state);

    switch ($form_state['section']) {
      case 'path':
        $form['path']['#description'] = t('This view will be displayed by visiting this path on your site. It is recommended that the path be something like "path/%/%/ical", putting one % in the path for each argument you have defined in the view.');
    }
  }

  /**
   * Instead of going through the standard views_view.tpl.php, delegate this
   * to the style handler.
   */
  function render() {
    return $this->view->style_plugin->render($this->view->result);
  }
}

