Fonctionnalités implémentées
Modes de couleur :

ColorMode.Black - Noir et blanc (1 bpp)
ColorMode.Gray4 - 4 niveaux de gris (2 bpp)
ColorMode.Gray16 - 16 niveaux de gris (4 bpp)
Orientations :

Orientation.Horizontal - 1200 × 825 pixels
Orientation.Vertical - 825 × 1200 pixels
Contrôle VCOM :

Lecture/écriture de la tension VCOM (spécifique à chaque écran)
Utilisation
CLI
Modifications apportées
@epaperjs/core : Ajout de ColorMode.Gray16
@epaperjs/cli :
Ajout du device rpi-9in7 dans le factory
Ajout des dépendances
Prochaines étapes pour finaliser
Sur Raspberry Pi, vous devrez :

Installer la bibliothèque bcm2835 :

```bash
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71
./configure && make && sudo make install
```

Installer les dépendances :

```bash
pnpm install
```

Compiler le driver natif :

```bash
cd packages/rpi-9in7
pnpm build
```

L'implémentation est complète et prête à être testée sur matériel réel. N'hésitez pas si vous avez des questions ou si vous souhaitez ajouter des fonctionnalités supplémentaires !

Exemple d'utilisation : 
```typescript
import { Rpi9In7 } from '@epaperjs/rpi-9in7';
import { Orientation, ColorMode } from '@epaperjs/core';
import * as fs from 'fs';

// Créer une instance avec VCOM personnalisé
const display = new Rpi9In7(
    Orientation.Horizontal,
    ColorMode.Gray16,
    -2.0  // Valeur VCOM (vérifier sur l'étiquette de l'écran)
);

// Connecter et initialiser
display.connect();
display.clear();

// Afficher une image PNG
const imageBuffer = fs.readFileSync('image.png');
await display.displayPng(imageBuffer);

// Mettre en veille et déconnecter
display.sleep();
display.disconnect();
```