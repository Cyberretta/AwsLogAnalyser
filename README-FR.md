# AwsLogAnalyser
## À quoi sert cet outil
Cet outil permet d'analyser des logs exportés depuis AWS au format Json avec une interface graphique. Il permet d'aider à réperer les activités suspectes. Cet outil est toujours en phase de test mais peut déjà être utiliser.

## Pourquoi cet outil
J'ai créé cet outil à l'origine pour m'aider dans les challenges d'enquêtes sur HackTheBox (comme Nubilum 1 et Nubilum 2). N'ayant pas trouver d'outils similaires, j'ai décidé d'en coder un moi-même.

## Comment ça fonctionne
Lors de l'ouverture du programme, vous devez selectionner un répertoire dans lequel se situent les fichiers logs au format .json. (le programme va chercher dans les sous répertoires de lui-même pour trouver tous les fichiers json). Le programme va mettre un certains temps à charger certaines données. Vous pourrez ensuite accéder à différents onglets :

### Events
L'onglet Event permet de créer des filters et d'afficher les events correspondant. Vous pouvez aussi filter les events par date. Le programem va automatiquement vous proposer des clés et des valeurs connus lorsque vous serez en train de compléter les champs.

### Statistics
L'onglet Statistics permet d'afficher des statistiques par rapport à une donnée précise. Par example, vous pouvez afficher le pourcentage d'events générer par chaque addresses IP sources.

### Errors
L'onglet Errors permet simplement de lister les erreurs présentes dans les logs.

### Alerts
L'onglet Alerts permet d'afficher les addresses IP potentiellement malveillantes. De plus, vous pourrez y retrouver les comptes utilisateur auquels ces addresses IP ont eu accès, ainsi que les politique IAM crées par ces addresses IP.
