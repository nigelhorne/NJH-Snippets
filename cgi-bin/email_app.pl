#!/usr/bin/perl
use strict;
use warnings;

BEGIN {
	# Sanitize environment variables
	delete @ENV{qw(IFS CDPATH ENV BASH_ENV)};
	$ENV{'PATH'} = '/usr/local/bin:/bin:/usr/bin';	# For insecurity

	if(-d '/home/hornenj/perlmods') {
		# Running at Dreamhost
		unshift @INC, (
			'/home/hornenj/perlmods/lib/perl/5.34',
			'/home/hornenj/perlmods/lib/perl/5.34.0',
			'/home/hornenj/perlmods/share/perl/5.34',
			'/home/hornenj/perlmods/share/perl/5.34.0',
			'/home/hornenj/perlmods/lib/perl5',
			'/home/hornenj/perlmods/lib/x86_64-linux-gnu/perl/5.34.0',
			'/home/hornenj/perlmods/lib/perl5/x86_64-linux-gnu-thread-multi'
		);
	}
}

no lib '.';

use CGI;
use Template;
use Email::Simple;
use Email::Sender::Simple qw(sendmail);
use Email::Sender::Transport::SMTP;
use Digest::SHA qw(sha256_hex);
use Data::Dumper;

# Configuration
my $SMTP_HOST = 'localhost';  # Change to your SMTP server
my $SMTP_PORT = 25;           # Change to your SMTP port
my $FROM_EMAIL = 'noreply@nigelhorne.com';  # Change to your domain
my $BASE_URL = 'https://nigelhorne.com/cgi-bin/email_app.pl';	# Change to your URL
my $DEBUG = 0;	# Set to 1 to enable debugging, 0 to disable

my $cgi = CGI->new;
my $tt = Template->new({
    INCLUDE_PATH => '.',
    INTERPOLATE  => 1,
}) || die $Template::ERROR;

# Simple session storage (in production, use proper session management)
my $session_file = '/tmp/email_sessions.dat';

print $cgi->header('text/html');

# Route handling
my $action = $cgi->param('action') || 'initial_form';

if ($action eq 'initial_form') {
    show_initial_form();
} elsif ($action eq 'send_verification') {
    send_verification_email();
} elsif ($action eq 'compose') {
    show_compose_form();
} elsif ($action eq 'send_email') {
    send_final_email();
} else {
    show_error("Invalid action");
}

sub show_initial_form {
    my $template = q{
<!DOCTYPE html>
<html>
<head>
    <title>Email Service - Step 1</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="email"], input[type="text"] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        input[type="submit"] { background-color: #007cba; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        input[type="submit"]:hover { background-color: #005a87; }
        .header { text-align: center; margin-bottom: 30px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Email Service</h1>
        <p>Step 1: Enter your details to receive a verification link</p>
    </div>
    
    <form method="post" action="">
        <input type="hidden" name="action" value="send_verification">
        
        <div class="form-group">
            <label for="email">Your Email Address:</label>
            <input type="email" id="email" name="email" required>
        </div>
        
        <div class="form-group">
            <label for="name">Your Name:</label>
            <input type="text" id="name" name="name" required>
        </div>
        
        <div class="form-group">
            <input type="submit" value="Send Verification Link">
        </div>
    </form>
</body>
</html>
    };
    
    $tt->process(\$template) || die $tt->error();
}

sub send_verification_email {
    my $email = $cgi->param('email');
    my $name = $cgi->param('name');
    
    unless ($email && $name) {
        show_error("Please provide both email and name");
        return;
    }
    
    # Generate verification token
    my $token = sha256_hex($email . time() . rand());
    
    # Store session data (in production, use proper database/session storage)
    store_session($token, { email => $email, name => $name, timestamp => time() });
    
    # Create verification link
    my $verify_link = "$BASE_URL?action=compose&token=$token";
    
    # Send verification email
    my $email_body = qq{
Hello $name,

You, or someone claiming to be you, has requested to send an email from this site.
If it was you, please click the link below.  If it was not, please disregard this message.

Please click the link below to compose and send your email:

$verify_link

This link will expire in 1 hour.

Best regards,
Email Service
    };
    
    eval {
        my $email_obj = Email::Simple->create(
            header => [
                To      => $email,
                From    => $FROM_EMAIL,
                Subject => 'Email Service - Verification Link',
            ],
            body => $email_body,
        );
        
        # Configure SMTP transport (adjust for your SMTP server)
        my $transport = Email::Sender::Transport::SMTP->new({
            host => $SMTP_HOST,
            port => $SMTP_PORT,
        });
        
        sendmail($email_obj, { transport => $transport });
    };
    
    if ($@) {
        show_error("Failed to send verification email: $@");
        return;
    }
    
    # Show success page
    my $template = q{
<!DOCTYPE html>
<html>
<head>
    <title>Verification Sent</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; text-align: center; }
        .success { color: #28a745; }
        .info { background-color: #d1ecf1; padding: 15px; border-radius: 4px; margin: 20px 0; }
    </style>
</head>
<body>
    <h1 class="success">Verification Email Sent!</h1>
    <div class="info">
        <p>We've sent a verification link to <strong>[% email %]</strong></p>
        <p>Please check your email and click the link to compose your message.</p>
        <p>The link will expire in 1 hour.</p>
    </div>
</body>
</html>
    };
    
    $tt->process(\$template, { email => $email }) || die $tt->error();
}

sub show_compose_form {
    my $token = $cgi->param('token');
    
    if ($DEBUG) {
        print STDERR "DEBUG: Token received: '$token'\n" if($token);
        print STDERR "DEBUG: Session file exists: " . (-f $session_file ? "YES" : "NO") . "\n";
        print STDERR "DEBUG: Session file path: $session_file\n";
    }
    
    unless ($token) {
        show_error('Invalid verification link - no token provided');
        return;
    }
    
    # Verify token and get session data
    my $session_data = get_session($token);
    
    if ($DEBUG) {
        print STDERR "DEBUG: Session data retrieved: " . (defined $session_data ? "YES" : "NO") . "\n";
        if ($session_data) {
            print STDERR "DEBUG: Session contains: email=" . ($session_data->{email} || "UNDEF") . 
                        ", name=" . ($session_data->{name} || "UNDEF") . 
                        ", timestamp=" . ($session_data->{timestamp} || "UNDEF") . "\n";
        }
    }
    
    unless ($session_data) {
        show_error("Invalid or expired verification link - session not found");
        return;
    }
    
    # Check if token is expired (1 hour)
    my $age = time() - $session_data->{timestamp};
    if ($DEBUG) {
        print STDERR "DEBUG: Session age: $age seconds (expires at 3600)\n";
    }
    
    if ($age > 3600) {
        show_error("Verification link has expired (age: " . int($age/60) . " minutes)");
        return;
    }

# Optional: Delete token immediately after first use (makes link single-use)
# Uncomment the next line if you want single-use verification links
# delete_session($token);
    
    my $template = q{
<!DOCTYPE html>
<html>
<head>
    <title>Compose Email - Step 2</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 700px; margin: 50px auto; padding: 20px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="email"], input[type="text"], textarea { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        textarea { height: 200px; resize: vertical; }
        input[type="submit"] { background-color: #28a745; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        input[type="submit"]:hover { background-color: #218838; }
        .header { text-align: center; margin-bottom: 30px; }
        .user-info { background-color: #f8f9fa; padding: 10px; border-radius: 4px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Compose Email</h1>
        <p>Step 2: Enter your email subject and message</p>
    </div>
    
    <div class="user-info">
        <strong>Sending as:</strong> [% session_data.name %] &lt;[% session_data.email %]&gt;
    </div>
    
    <form method="post" action="">
        <input type="hidden" name="action" value="send_email">
        <input type="hidden" name="token" value="[% token %]">
        
	<!--
        <div class="form-group">
            <label for="to_email">To Email Address:</label>
            <input type="email" id="to_email" name="to_email" required>
        </div>
	-->
        
        <div class="form-group">
            <label for="subject">Subject:</label>
            <input type="text" id="subject" name="subject" required>
        </div>
        
        <div class="form-group">
            <label for="message">Message:</label>
            <textarea id="message" name="message" required placeholder="Enter your email message here..."></textarea>
        </div>
        
        <div class="form-group">
            <input type="submit" value="Send Email">
        </div>
    </form>
</body>
</html>
    };
    
    $tt->process(\$template, { token => $token, session_data => $session_data }) || die $tt->error();
}

sub send_final_email {
    my $token = $cgi->param('token');
    my $to_email = $cgi->param('to_email') || 'njh@nigelhorne.com';
    my $subject = $cgi->param('subject');
    my $message = $cgi->param('message');
    
    unless ($token && $to_email && $subject && $message) {
        show_error("All fields are required");
        return;
    }
    
    # Verify token and get session data
    my $session_data = get_session($token);
    unless ($session_data) {
        show_error("Invalid or expired verification link");
        return;
    }
    
    # Check if token is expired
    if (time() - $session_data->{timestamp} > 3600) {
        show_error("Verification link has expired");
        return;
    }
    
    # Send the email
    eval {
        my $email_obj = Email::Simple->create(
            header => [
                To      => $to_email,
                From    => "$session_data->{name} <$session_data->{email}>",
                Subject => $subject,
                'Reply-To' => $session_data->{email},
            ],
            body => $message,
        );
        
        my $transport = Email::Sender::Transport::SMTP->new({
            host => $SMTP_HOST,
            port => $SMTP_PORT,
        });
        
        sendmail($email_obj, { transport => $transport });
    };
    
    if ($@) {
        show_error("Failed to send email: $@");
        return;
    }
    
    # Clean up session
    delete_session($token);
    
    # Show success page
    my $template = q{
<!DOCTYPE html>
<html>
<head>
    <title>Email Sent Successfully</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; text-align: center; }
        .success { color: #28a745; }
        .info { background-color: #d4edda; padding: 15px; border-radius: 4px; margin: 20px 0; }
        a { color: #007cba; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1 class="success">Email Sent Successfully!</h1>
    <div class="info">
    	<--
        <p>Your email has been sent to <strong>[% to_email %]</strong></p>
	-->
        <p>Your email has been sent</p>
        <p>Subject: <strong>[% subject %]</strong></p>
    </div>
    <p><a href="[% base_url %]">Send Another Email</a></p>
</body>
</html>
    };
    
    $tt->process(\$template, { 
        to_email => $to_email, 
        subject => $subject,
        base_url => $BASE_URL
    }) || die $tt->error();
}

sub store_session {
    my ($token, $data) = @_;
    
    my $sessions = {};
    if (-f $session_file) {
        eval {
            open my $fh, '<', $session_file or die "Can't read session file: $!";
            local $/;
            my $content = <$fh>;
            close $fh;
            if ($content && $content =~ /\S/) {
                my $VAR1;  # For Data::Dumper output
                $sessions = eval $content;
                $sessions = {} unless ref $sessions eq 'HASH';
            }
        };
        # If eval fails, start with empty sessions hash
        $sessions = {} if $@;
    }
    
    $sessions->{$token} = $data;
    
    # Use a more reliable serialization method
    eval {
        open my $fh, '>', $session_file or die "Can't write session file: $!";
        my $dumper = Data::Dumper->new([$sessions]);
        $dumper->Purity(1);
        $dumper->Terse(1);
        print $fh $dumper->Dump();
        close $fh;
        chmod 0600, $session_file;  # Secure the file
    };
    die "Failed to store session: $@" if $@;
}

sub get_session {
    my ($token) = @_;
    
    return undef unless -f $session_file;
    return undef unless $token;
    
    my $sessions = {};
    eval {
        open my $fh, '<', $session_file or die "Can't read session file: $!";
        local $/;
        my $content = <$fh>;
        close $fh;
        
        if ($content && $content =~ /\S/) {
            my $VAR1;  # For Data::Dumper output
            $sessions = eval $content;
            $sessions = {} unless ref $sessions eq 'HASH';
        }
    };
    
    # Return undef if there was an error or token doesn't exist
    return undef if $@ || !exists $sessions->{$token};
    return $sessions->{$token};
}

sub delete_session {
    my ($token) = @_;
    
    return unless -f $session_file;
    return unless $token;
    
    my $sessions = {};
    eval {
        open my $fh, '<', $session_file or die "Can't read session file: $!";
        local $/;
        my $content = <$fh>;
        close $fh;
        
        if ($content && $content =~ /\S/) {
            my $VAR1;  # For Data::Dumper output
            $sessions = eval $content;
            $sessions = {} unless ref $sessions eq 'HASH';
        }
    };
    
    return if $@;
    
    delete $sessions->{$token};
    
    eval {
        open my $fh, '>', $session_file or die "Can't write session file: $!";
        my $dumper = Data::Dumper->new([$sessions]);
        $dumper->Purity(1);
        $dumper->Terse(1);
        print $fh $dumper->Dump();
        close $fh;
    };
}

sub show_error {
    my ($error_msg) = @_;
    
    my $template = q{
<!DOCTYPE html>
<html>
<head>
    <title>Error</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; text-align: center; }
        .error { color: #dc3545; background-color: #f8d7da; padding: 15px; border-radius: 4px; margin: 20px 0; }
        a { color: #007cba; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>Error</h1>
    <div class="error">
        <p>[% error_msg %]</p>
    </div>
    <p><a href="[% base_url %]">Start Over</a></p>
</body>
</html>
    };
    
    $tt->process(\$template, { 
        error_msg => $error_msg,
        base_url => $BASE_URL
    }) || die $tt->error();
}
