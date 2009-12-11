<?php
// $Id$
?>
<div class="oauthloginclient-index">
  <ul class="oauthlogin-server-list">
    <?php if (empty($servers)): ?>
      <li class="no-servers"><?php print t('No servers added'); ?></li>
    <?php endif ?>
    <?php foreach ($servers as $sid => $info): ?>
      <li class="server">
        <div class="server-name"><?php print l($info['title'], 'admin/settings/oauthloginclient/server/' . $sid) ?></div>
        <a class="address"><?php print l($info['url'], $info['url']); ?></a>
      </li>
    <?php endforeach ?>
  </ul>
</div>