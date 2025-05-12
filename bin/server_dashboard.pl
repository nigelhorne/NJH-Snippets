#!/usr/bin/env perl
use strict;
use warnings;
use Mojolicious::Lite;
use LWP::UserAgent;
use Time::HiRes qw(time);
use File::Basename;

# Log to a file for errors
sub log_error {
    my ($url, $status, $message) = @_;
    my $log_file = 'server_status.log';
    my $timestamp = localtime();
    open my $log_fh, '>>', $log_file or die "Cannot open log file: $!";
    print $log_fh "$timestamp - $status - $url - $message\n";
    close $log_fh;
}

# Read server list from config file
sub load_servers {
    my $file = 'servers.txt';
    open my $fh, '<', $file or die "Cannot open $file: $!";
    my @list = grep { /\S/ } map { chomp; $_ } <$fh>;
    close $fh;
    return @list;
}

# Route: dashboard with optional filter
get '/' => sub {
    my $c = shift;
    my $filter = $c->param('filter') || 'all';  # all, ok, error

    my $ua = LWP::UserAgent->new(timeout => 5);
    my @servers = load_servers();

    my @statuses;
    foreach my $url (@servers) {
        my ($res, $status, $color, $code, $icon, $duration, $tooltip);

        my $start = time;
        eval {
            $res = $ua->get($url);
        };
        $duration = int(1000 * (time - $start));  # in ms

        if ($@ || !$res || ($res->code == 500 && $res->decoded_content =~ /Can't connect|Name or service not known|timed out/i)) {
            $status  = 'Unreachable';
            $color   = 'red';
            $icon    = '🔴';
            $code    = '';
            $tooltip = $@ || ($res ? $res->status_line : 'No response');
            log_error($url, 'Unreachable', $tooltip);
        } elsif ($res->is_success) {
            $status  = 'OK';
            $color   = 'green';
            $icon    = '🟢';
            $code    = $res->code;
            $tooltip = $res->status_line;
        } else {
            $status  = 'HTTP Error';
            $color   = 'orange';
            $icon    = '🟡';
            $code    = $res->code;
            $tooltip = $res->status_line;
            log_error($url, 'HTTP Error', $tooltip);
        }

        my $time_color = $duration < 500   ? 'green'
                        : $duration < 1000 ? 'orange'
                        :                    'red';

        push @statuses, {
            url        => $url,
            status     => $status,
            color      => $color,
            code       => $code,
            icon       => $icon,
            duration   => $duration,
            time_color => $time_color,
            tooltip    => $tooltip,
        };
    }

    # Apply filter
    if ($filter eq 'ok') {
        @statuses = grep { $_->{color} eq 'green' } @statuses;
    } elsif ($filter eq 'error') {
        @statuses = grep { $_->{color} ne 'green' } @statuses;
    }

    $c->stash(statuses => \@statuses, filter => $filter);
    $c->render(template => 'dashboard');
};

app->start;

__DATA__

@@ dashboard.html.ep
% layout 'default';
% title 'Web Server Dashboard';
<h1>Web Server Status</h1>
<p><em>Auto-refreshes every 60 seconds</em></p>

<form method="get" action="/">
  <label for="filter">Filter:</label>
  <select name="filter" onchange="this.form.submit()">
    <option value="all"   <%= 'selected' if $filter eq 'all' %>>All</option>
    <option value="ok"    <%= 'selected' if $filter eq 'ok' %>>Only OK</option>
    <option value="error" <%= 'selected' if $filter eq 'error' %>>Only Errors</option>
  </select>
</form>

<table border="1" cellpadding="6" cellspacing="0">
  <tr>
    <th>Server</th>
    <th>Status</th>
    <th>HTTP Code</th>
    <th>Response Time</th>
  </tr>
  % for my $s (@$statuses) {
    <tr>
      <td><a href="<%= $s->{url} %>" target="_blank"><%= $s->{url} %></a></td>
      <td style="color:<%= $s->{color} %>;" title="<%= $s->{tooltip} // '' %>">
        <b><%= $s->{icon} %> <%= $s->{status} %></b>
      </td>
      <td><%= $s->{code} || '-' %></td>
      <td style="color:<%= $s->{time_color} %>;"><%= $s->{duration} %> ms</td>
    </tr>
  % }
</table>

@@ layouts/default.html.ep
<!DOCTYPE html>
<html>
  <head>
    <title><%= title %></title>
    <meta http-equiv="refresh" content="60">
    <style>
      body { font-family: sans-serif; padding: 20px; }
      table { border-collapse: collapse; width: 100%; }
      th, td { padding: 6px 12px; text-align: left; }
      th { background: #f0f0f0; }
      a { text-decoration: none; color: blue; }
      select { margin: 8px; padding: 4px; }
    </style>
  </head>
  <body><%= content %></body>
</html>
