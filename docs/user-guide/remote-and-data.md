# Connect a remote

Use the phone icon in the toolbar to open Remote. Select your local network interface and Start remote. Open the shown address or QR code on a device connected to the same trusted network, then use the displayed pairing details. Allow local network access if your operating system asks.

A 127.0.0.1 address works only on this computer. Choose the Wi-Fi or Ethernet address for another device.

## Keep your work

Songs, imported Bibles, templates, programs, and preferences are stored locally. Imported Bibles remain usable offline. Use available export controls to keep copies of important work and media. Google Fonts must be downloaded before offline use.

The Selah rename preserves the original ChurchDeck / Church Deck storage namespace. Do not delete that folder while cleaning up old installations.

## Full backup and restore

Choose File → **Backup & restore…** → **Export full backup…**. Save the `.selahbackup` outside Selah's own data folder. It includes your managed libraries, imported Bibles, media, programs, presentations, templates, downloaded fonts/models and preferences. Referenced external media files are included too. Partial downloads, database journals and lock files are excluded.

To restore, choose a backup and select **Prepare restore**. After verification, choose **Quit to restore**, then reopen Selah. Your current data is preserved in a sibling folder named `.before-restore-…`. You can cancel a prepared restore before quitting. All outputs start off; a saved interrupted session still needs explicit confirmation. Platform-specific Luma runtimes may need reconfiguration when moving between operating systems.

## Transfer one service

Use **Export → Portable bundle with media…** in an open program, or File → **Export portable program…**. Import the `.selahbundle` through **Import program…** on the other computer. Its media and template snapshots are included. This adds an independent program without replacing libraries or taking it live. Install matching fonts on the destination before presenting.
